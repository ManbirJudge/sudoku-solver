#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/geometry/2d.hpp>

#include <tensorflow/lite/c/common.h>
#include <tensorflow/lite/c/c_api.h>

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <solver.hpp>

#define TAG "SudokuNativeLib"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define OK                  0
#define PAPER_NOT_FOUND     1
#define LESSER_CELLS_FOUND  2
#define GREATER_CELLS_FOUND 3
#define IMPOSSIBLE          5

// ---
namespace chrono = std::chrono;

using Point   = cv::Point;
using Point2f = cv::Point2f;
using Point2i = cv::Point2i;
using Scalar  = cv::Scalar;
using Rect    = cv::Rect;
using Size    = cv::Size;
using Mat     = cv::Mat;

using Contour = std::vector<Point>;
using Polygon = std::vector<Point>;

// constants
const int    TARGET_FPS = 40;
const double TARGET_WAIT_TIME = (1. / TARGET_FPS) * 1000.;

const int PAPER_S = 810;
const int CELL_S = 128;
const int PAD = 14;

Scalar RED(255, 0, 0, 255);
Scalar GREEN(0, 255, 0, 255);

// ---
static std::vector<char> g_model_buf;
static TfLiteInterpreter* g_interpreter = nullptr;

static std::vector<float> g_in_buf;

static auto g_prev_t = chrono::steady_clock::now();

// utils
namespace Utils {
    Point2f p2i_2_p2f(Point2i pt) {
        return {(float) pt.x, (float) pt.y};
    }

    Point get_center(const Polygon &poly) {
        int sX = 0, sY = 0;

        for (const Point pt: poly) {
            sX += pt.x;
            sY += pt.y;
        }

        return {static_cast<int>(sX / poly.size()), static_cast<int>(sY / poly.size())};
    }

    bool reorder_quad_for_warp(std::vector<Point> quad, Point2f src[4]) {
        if (quad.size() != 4)
            return false;

        int sum[4], diff[4];

        for (int i = 0; i < 4; i++) {
            sum[i] = quad[i].x + quad[i].y;
            diff[i] = quad[i].x - quad[i].y;
        }

        src[0] = p2i_2_p2f(quad[std::min_element(sum, sum + 4) - sum]);
        src[1] = p2i_2_p2f(quad[std::max_element(diff, diff + 4) - diff]);
        src[2] = p2i_2_p2f(quad[std::min_element(diff, diff + 4) - diff]);
        src[3] = p2i_2_p2f(quad[std::max_element(sum, sum + 4) - sum]);

        return true;
    }

    void sort_contours(std::vector<Contour> &contours) {
        std::vector<std::pair<Contour, Rect>> cntRects;

        for (const auto &cnt: contours) {
            cntRects.emplace_back(cnt, boundingRect(cnt));
        }

        sort(
                cntRects.begin(),
                cntRects.end(),
                [](const auto &a, const auto &b) {
                    int dy = a.second.y - b.second.y;

                    if (abs(dy) > 10)
                        return a.second.y < b.second.y;
                    return a.second.x < b.second.x;
                }
        );

        for (size_t i = 0; i < cntRects.size(); i++)
            contours[i] = cntRects[i].first;
    }

    bool is_cell_empty(const Mat &cell, Rect &bounding_rect) {
            const double area_min = 0.05 * cell.size().area();
            const double area_max = 0.8 * cell.size().area();

            std::vector<Contour> contours;
            cv::findContours(cell, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

            if (contours.size() < 2) return true;

            for (const Contour &cnt: contours) {
                bounding_rect = cv::boundingRect(cnt);

                if (bounding_rect.area() >= area_min && bounding_rect.area() <= area_max) return false;
            }

            return true;
        }
}

// digit model functions
bool init_digit_model() {
    if (g_interpreter) {
        LOGW("Model already initialized.");
        return true;
    }

    // ---
    TfLiteModel* model = TfLiteModelCreate(g_model_buf.data(), g_model_buf.size());
    if (!model) {
        LOGE("Failed to create model.");
        return false;
    }

    // interpreter
    TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 2);

    g_interpreter = TfLiteInterpreterCreate(model, options);

    TfLiteModelDelete(model);
    TfLiteInterpreterOptionsDelete(options);

    if (!g_interpreter) {
        LOGE("Failed to create interpreter.");
        return false;
    }

    // allocating tensors
    if (TfLiteInterpreterAllocateTensors(g_interpreter) != kTfLiteOk) {
        LOGE("Failed to allocate tensors for interpreter.");
        TfLiteInterpreterDelete(g_interpreter);
        g_interpreter = nullptr;
        return false;
    }

    // ---
    LOGI("Initialized digit model.");
    return true;
}

bool recognize_digits(const std::vector<Mat> &imgs, Paper &digits) {
    digits.fill(0);

    if (!g_interpreter){
        LOGE("Digit model not initialized.");
        return false;
    }

    // ---
    TfLiteTensor *in_tensor = TfLiteInterpreterGetInputTensor(g_interpreter, 0);

    int req_batch_size = TfLiteTensorDim(in_tensor, 0);
    int req_h =          TfLiteTensorDim(in_tensor, 1);
    int req_w =          TfLiteTensorDim(in_tensor, 2);
    int req_n_channels = TfLiteTensorDim(in_tensor, 3);

    if (req_batch_size > 0 && req_batch_size != 81) {
        LOGE("Input batch size must be flexible or 81, nothing else. Current is: %i", req_batch_size);
        return false;
    }

    // allocating input buffer
    size_t cell_size = req_w * req_h * req_n_channels;
    g_in_buf.resize(81 * cell_size);

    // pre-processing
    for (int i = 0; i < 81; i++) {
        Mat in_cell = imgs[i];

        cv::resize(in_cell, in_cell, Size(req_h, req_w));
        in_cell.convertTo(in_cell, CV_32F, 1. / 255.);

        if (in_cell.channels() != req_n_channels) {
            LOGE("Invalid number of channels provided.");
            return false;
        }

        std::memcpy(g_in_buf.data() + i * cell_size, in_cell.ptr<float>(), cell_size * sizeof(float));
    }

    // copying data into input tensor
    if (TfLiteTensorCopyFromBuffer(in_tensor, g_in_buf.data(), 81 * cell_size * sizeof(float)) != kTfLiteOk) {
        LOGE("Failed to copy data to input tensor.");
        return false;
    }

    //  inference
    if (TfLiteInterpreterInvoke(g_interpreter) != kTfLiteOk) {
        LOGE("Failed to invoke interpreter.");
        return false;
    }

    // getting output
    const TfLiteTensor *out_tensor = TfLiteInterpreterGetOutputTensor(g_interpreter, 0);

    int out_batch_size = TfLiteTensorDim(out_tensor, 0);
    int out_n =          TfLiteTensorDim(out_tensor, 1);

    if (out_batch_size != 81) {
        LOGE("Interpreter output batch size not 81; it is: %i.", out_batch_size);
        return false;
    }
    if (out_n != 9) {
        LOGE("Interpreter output doesn't contain 9 values.");
        return false;
    }

    std::vector<float> predictions(9 * 81);

    if (TfLiteTensorCopyToBuffer(out_tensor, predictions.data(), 9 * 81 * sizeof(float)) != kTfLiteOk) {
        LOGE("Failed to copy output data from tensor.");
        TfLiteInterpreterDelete(g_interpreter);
        return false;
    }

    for (int i = 0; i < 81; i++) {
        auto first = predictions.begin() + i * 9;
        auto last = first + 9;
        digits[i] = (int)(std::max_element(first, last) - first) + 1;
    }

    // ---
    return true;
}

void cleanup_digit_model() {
    if (g_interpreter) {
        TfLiteInterpreterDelete(g_interpreter);
        g_interpreter = nullptr;
    }
    g_model_buf.clear();

    LOGD("Digit model cleaned.");
}

// actual functions
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_sudokusolver_ImageAnalyzer_processFrame(
    JNIEnv */*env*/,
    jobject /*self*/,
    jlong pMat
) {
    // ---
    auto cur_t = chrono::high_resolution_clock::now();
    auto dt = chrono::duration_cast<chrono::microseconds>(cur_t - g_prev_t).count() / 1000;

    if (dt <= TARGET_WAIT_TIME) return false;

    // ---
    Mat &out = *reinterpret_cast<Mat*>(pMat);
    Mat frame = out.clone();

    int frame_area = frame.cols * frame.rows;

    // --- pre-processing image
    cv::cvtColor(frame, frame, cv::COLOR_RGBA2GRAY);
    cv::GaussianBlur(frame, frame, Size(5, 5), 2);
    cv::adaptiveThreshold(frame, frame, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

    // --- contours
    std::vector<Contour> contours;
    cv::findContours(frame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // --- biggest quad
    Polygon biggest_quad(4);

    double _cnt_area_min = 0.1 * frame_area;
    double _biggest_quad_area = 0;
    for (const Contour &cnt: contours) {
        double _area = cv::contourArea(cnt);
        if (_area < _cnt_area_min) continue;

        std::vector<Point> poly;
        double _arc_len = cv::arcLength(cnt, true);
        cv::approxPolyDP(cnt, poly, 0.02 * _arc_len, true);

        if (poly.size() == 4) {
            if (_area > _biggest_quad_area) {
                biggest_quad = poly;
                _biggest_quad_area = _area;
            }
        }
    }

    // --- drawing
    if (_biggest_quad_area > 0)
        cv::polylines(out,biggest_quad,true,RED,2,cv::LINE_AA);

    int fps = (int)(1000. / (float)dt);
    putText(
        out,
        (std::to_string(fps) + " FPS"),
        Point(10, 16),
        cv::FONT_HERSHEY_SIMPLEX,
        0.75,
        RED,
        1,
        cv::LINE_AA
    );

    // ---
    frame.release();

    g_prev_t = chrono::high_resolution_clock::now();

    return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_sudokusolver_CameraFragment_initializeLib(
    JNIEnv *env,
    jobject /*self*/,
    jobject assetMgr
) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetMgr);

    AAsset *digit_model_asset = AAssetManager_open(mgr, "digit-model.tflite", AASSET_MODE_BUFFER);
    if (!digit_model_asset) {
        LOGE("Failed to open digit model asset.");
        return false;
    }

    size_t model_size = AAsset_getLength(digit_model_asset);
    g_model_buf.resize(model_size);
    if (AAsset_read(digit_model_asset, g_model_buf.data(), model_size) != model_size) {
        LOGE("Failed to read digit model asset.");
        return false;
    }

    init_digit_model();

    AAsset_close(digit_model_asset);

    LOGI("Native library initialized.");
    return true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_sudokusolver_CameraFragment_cleanupLib(
    JNIEnv* /*env*/,
    jobject /*self*/
) {
    cleanup_digit_model();
    LOGI("Native library cleaned.");
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_example_sudokusolver_CameraFragment_processFrame(
    JNIEnv *env,
    jobject /*self*/,
    jlong pMat
) {
    auto p1 = chrono::high_resolution_clock::now();

    // ---
    jclass resClass = env->FindClass("com/example/sudokusolver/FrameProcessingRes");
    if (resClass == nullptr) return nullptr;

    jmethodID ctor = env->GetMethodID(
        resClass,
        "<init>",
        "(IJJJJJJ)V"
    );
    if (ctor == nullptr) return nullptr;

    // ---
    Mat &out = *reinterpret_cast<Mat*>(pMat);
    Mat frame = out.clone();

    Mat paper_original(Size(PAPER_S, PAPER_S), frame.type());
    Mat paper(Size(PAPER_S, PAPER_S), frame.type());
    Mat paper_thresh(Size(PAPER_S, PAPER_S), frame.type());

    int frame_area = frame.cols * frame.rows;

    auto p2 = chrono::high_resolution_clock::now();

    // --- pre-processing image
    cv::cvtColor(frame, frame, cv::COLOR_RGBA2GRAY);
    cv::GaussianBlur(frame, frame, Size(5, 5), 2);
    cv::adaptiveThreshold(frame, frame, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

    // --- contours
    std::vector<Contour> contours;
    cv::findContours(frame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // --- biggest quad
    Polygon biggest_quad(4);

    double _cnt_area_min = 0.1 * frame_area;
    double _biggest_quad_area = 0;
    for (const Contour &cnt: contours) {
        double _area = cv::contourArea(cnt);
        if (_area < _cnt_area_min) continue;

        std::vector<Point> poly;
        double _arc_len = cv::arcLength(cnt, true);
        cv::approxPolyDP(cnt, poly, 0.02 * _arc_len, true);

        if (poly.size() == 4) {
            if (_area > _biggest_quad_area) {
                biggest_quad = poly;
                _biggest_quad_area = _area;
            }
        }
    }

    if (_biggest_quad_area == 0)
        return env->NewObject(resClass, ctor, PAPER_NOT_FOUND, 0l, 0l, 0l, 0l, 0l, 0l);

    auto p3 = chrono::high_resolution_clock::now();

    // --- getting paper
    Point2f warp_src[4];
    Point2f warp_dst[4] = {
        {0.f,             0.f},
        {(float) PAPER_S, 0.f},
        {0.f,             (float) PAPER_S},
        {(float) PAPER_S, (float) PAPER_S}
    };

    Utils::reorder_quad_for_warp(biggest_quad, warp_src);
    Mat paper_warp_mat = cv::getPerspectiveTransform(warp_src, warp_dst);

    cv::warpPerspective(out, paper_original, paper_warp_mat, Point(PAPER_S, PAPER_S));

    // --- pre-processing paper
    cv::cvtColor(paper_original, paper, cv::COLOR_RGBA2GRAY);
    cv::adaptiveThreshold(paper, paper, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 57, 5);

    paper.copyTo(paper_thresh);

    // --- processing paper
    // contours
    contours.clear();
    cv::findContours(paper, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // removing small contours
    std::vector<Contour> contours_to_remove;

    for (const Contour &cnt: contours) {
        double _area = cv::contourArea(cnt);
        if (_area < frame_area / 100.)
            contours_to_remove.push_back(cnt);
    }

    cv::drawContours(paper, contours_to_remove, -1, Scalar(0), -1, cv::LINE_AA);

    // removing (still remaining) speckles
    Mat opening_kernel = cv::getStructuringElement(cv::MORPH_RECT, Point(3, 3));
    cv::morphologyEx(paper, paper, cv::MORPH_OPEN, opening_kernel);

    // fixing lines
    Mat v_kernel = cv::getStructuringElement(cv::MORPH_RECT, Point(1, 5));
    Mat h_kernel = cv::getStructuringElement(cv::MORPH_RECT, Point(5, 1));

    cv::morphologyEx(paper, paper, cv::MORPH_CLOSE, v_kernel, Point(-1, -1), 4);
    cv::morphologyEx(paper, paper, cv::MORPH_CLOSE, h_kernel, Point(-1, -1), 4);

    // extracting cells
    std::vector<Polygon> cell_polys;
    std::vector<Mat> cells;
    std::vector<bool> cell_emptys;

    paper = 255 - paper;
    paper_thresh = 255 - paper_thresh;   // TODO: check validity

    contours.clear();
    cv::findContours(paper, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    Utils::sort_contours(contours);

    for (const Contour &cnt: contours) {  // getting cell polygons
        Polygon approx;

        double arc_len = cv::arcLength(cnt, true);
        cv::approxPolyDP(cnt, approx, 0.02 * arc_len, true);

        if (approx.size() == 4) cell_polys.push_back(approx);
    }

    LOGD("Found %i cells.", (int)cell_polys.size());

    if (cell_polys.size() < 81) return env->NewObject(resClass, ctor, LESSER_CELLS_FOUND, 0l, 0l, 0l, 0l, 0l, 0l);
    else if (cell_polys.size() > 81) return env->NewObject(resClass, ctor, GREATER_CELLS_FOUND, 0l, 0l, 0l, 0l, 0l, 0l);

    warp_dst[0] = {0.f,            0.f};
    warp_dst[1] = {(float) CELL_S, 0.f};
    warp_dst[2] = {0.f,            (float) CELL_S};
    warp_dst[3] = {(float) CELL_S, (float) CELL_S};
    Rect _bounding_rect;
    for (const auto &cell_poly : cell_polys) {  // getting the image from cell polygons
        Mat cell(CELL_S, CELL_S, paper_thresh.type());

        Utils::reorder_quad_for_warp(cell_poly, warp_src);
        Mat cell_warp_mat = cv::getPerspectiveTransform(warp_src, warp_dst);
        cv::warpPerspective(paper_thresh, cell, cell_warp_mat, Point(CELL_S, CELL_S));

        bool is_empty = Utils::is_cell_empty(cell, _bounding_rect);

        if (!is_empty) {  // TODO: understand
            Rect cropRect(
                std::max(_bounding_rect.x - PAD, 0),
                std::max(_bounding_rect.y - PAD, 0),
                std::min(_bounding_rect.width  + 2*PAD, cell.cols - std::max(_bounding_rect.x - PAD, 0)),
                std::min(_bounding_rect.height + 2*PAD, cell.rows - std::max(_bounding_rect.y - PAD, 0))
            );

            Mat digitCrop = cell(cropRect).clone();

            int top    = std::max(0, PAD - _bounding_rect.y);
            int left   = std::max(0, PAD - _bounding_rect.x);
            int bottom = std::max(0, (_bounding_rect.y + _bounding_rect.height + PAD) - cell.rows);
            int right  = std::max(0, (_bounding_rect.x + _bounding_rect.width  + PAD) - cell.cols);

            copyMakeBorder(digitCrop, cell, top, bottom, left, right, cv::BORDER_CONSTANT, Scalar(0));
        }

        cells.push_back(cell);
        cell_emptys.push_back(is_empty);
    }

    auto p4 = chrono::high_resolution_clock::now();

    // --- digit recognition
    std::array<int, 81> digits{};

    recognize_digits(cells, digits);
    for (int i = 0; i < 81; i++)
        if (cell_emptys[i]) digits[i] = 0;

    auto p5 = chrono::high_resolution_clock::now();

    // --- solution
    bool solved = solve(digits);
    auto p6 = chrono::high_resolution_clock::now();

    // --- drawing (on original and then inverse-warping it)
    for (size_t i = 0; i < cell_polys.size(); i++) {
        const Polygon &poly = cell_polys[i];
        const Point centre = Utils::get_center(poly);

        if (digits[i] == 0) continue;

        std::string txt = std::to_string(digits[i]);

        const Size txt_size = cv::getTextSize(txt, cv::FONT_HERSHEY_SIMPLEX, 3, 2, nullptr);
        const Point org(
            centre.x - txt_size.width / 2,
            centre.y + txt_size.height / 2
        );

        cv::putText(paper_original, txt, org, cv::FONT_HERSHEY_SIMPLEX, 3, cell_emptys[i] ? RED : GREEN, 2, cv::LINE_AA);
    }

    Mat paper_inv_warp_mat;
    cv::invert(paper_warp_mat, paper_inv_warp_mat);

    cv::warpPerspective(paper_original, out, paper_inv_warp_mat, out.size(), cv::INTER_CUBIC, cv::BORDER_TRANSPARENT);

    auto p7 = chrono::high_resolution_clock::now();

    // --- cleanup
    frame.release();
    paper_original.release();
    paper.release();
    paper_thresh.release();

    auto p8 = chrono::high_resolution_clock::now();

    // --- result
    auto processingImgT = chrono::duration_cast<chrono::milliseconds>(p3 - p2).count();
    auto processingPprT = chrono::duration_cast<chrono::milliseconds>(p4 - p3).count();
    auto digitRecogT =    chrono::duration_cast<chrono::milliseconds>(p5 - p4).count();
    auto solnT =          chrono::duration_cast<chrono::microseconds>(p6 - p5).count();
    auto drawingT =       chrono::duration_cast<chrono::milliseconds>(p7 - p6).count();
    auto totalT =         chrono::duration_cast<chrono::milliseconds>(p8 - p1).count();

    return env->NewObject(resClass, ctor,
         solved ? OK : IMPOSSIBLE,
         processingImgT,
         processingPprT,
         digitRecogT,
         solnT,
         drawingT,
         totalT
    );
}