#include <chrono>
#include <vector>
#include <string>
#include <algorithm>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <tensorflow/lite/c/common.h>
#include <tensorflow/lite/c/c_api.h>

#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "base.h"
#include "solver.h"

#define OK                  0
#define PAPER_NOT_FOUND     1
#define LESSER_CELLS_FOUND  2
#define GREATER_CELLS_FOUND 3
#define INVALID_SUDOKU      4
#define COULDNT_SOLVE       5

// types
using namespace cv;

typedef vector<Point> Cnt;
typedef vector<Point> Poly;

// constants
const int FPS = 40;
const float WAIT_TIME = (1.f / (float)FPS) * 1000.f;

const float AREA_MIN_REL = 0.1f;
const float CELL_EMPTY_THRESH = 0.05f;
const int PAPER_S = 810;
const int CELL_S = 128;
//const size_t MAX_COUNT = 82;
const int PAD = 14;

Scalar RED(255, 0, 0, 255);
Scalar GREEN(0, 255, 0, 255);

// ---
static vector<char> modelBuff;
static TfLiteInterpreter* interpreter = nullptr;

static vector<float> inBuff;

// ---
static auto prevT = chrono::steady_clock::now();

// utils
Point2f p2i2p2f(Point2i pt) {
    return {(float) pt.x, (float) pt.y};
}

Point getCentre(const Poly &poly) {
    int sX = 0, sY = 0;

    for (const Point pt: poly) {
        sX += pt.x;
        sY += pt.y;
    }

    return {static_cast<int>(sX / poly.size()), static_cast<int>(sY / poly.size())};
}

bool reorderQuadForWarp(vector<Point> quad, Point2f src[4]) {
    if (quad.size() != 4)
        return false;

    int sum[4], diff[4];

    for (int i = 0; i < 4; i++) {
        sum[i] = quad[i].x + quad[i].y;
        diff[i] = quad[i].x - quad[i].y;
    }

    src[0] = p2i2p2f(quad[min_element(sum, sum + 4) - sum]);
    src[1] = p2i2p2f(quad[max_element(diff, diff + 4) - diff]);
    src[2] = p2i2p2f(quad[min_element(diff, diff + 4) - diff]);
    src[3] = p2i2p2f(quad[max_element(sum, sum + 4) - sum]);

    return true;
}

void sortContours(vector<Cnt> &contours) {
    vector<pair<Cnt, Rect>> cntRects;

    for (const auto &cnt: contours) {
        cntRects.emplace_back(cnt, boundingRect(cnt));
    }

    sort(
        cntRects.begin(),
        cntRects.end(),
        [](const pair<Cnt, Rect> &a, const pair<Cnt, Rect> &b) {
                int dy = a.second.y - b.second.y;

                if (abs(dy) > 10)
                    return a.second.y < b.second.y;
                return a.second.x < b.second.x;
            }
    );

    for (size_t i = 0; i < cntRects.size(); i++)
        contours[i] = cntRects[i].first;
}

bool isCellEmpty(const Mat& cell)
{
    int nonZero = countNonZero(cell);
    return nonZero > (1 - CELL_EMPTY_THRESH) * cell.size().area();
}

bool isCellEmpty2(const Mat& cell, Rect& boundingRect)
{
    double areaMin = CELL_EMPTY_THRESH * cell.size().area();
    double areaMax = 0.8 * cell.size().area();

    vector<Cnt> contours;
    findContours(cell, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);

    if (contours.size() < 2)
        return true;

    for (const Cnt &cnt: contours) {
        boundingRect = cv::boundingRect(cnt);

        if (boundingRect.area() >= areaMin && boundingRect.area() <= areaMax)
            return false;
    }

    return true;
}

// digit model functions
bool initDigitModel(AAsset *digitModelAsset)
{
    if (interpreter != nullptr)
    {
        LOGD("Model already initialized.");
        return true;
    }

    // loading data
    size_t digitModelAssLen = AAsset_getLength(digitModelAsset);

    modelBuff.resize(digitModelAssLen);
    if (AAsset_read(digitModelAsset, modelBuff.data(), digitModelAssLen) != digitModelAssLen) {
        LOGE("Failed to read model asset.");
        return false;
    }

    // ---
    TfLiteModel* model = TfLiteModelCreate(modelBuff.data(), digitModelAssLen);
    if (!model) {
        LOGE("Failed to load model.");
        return false;
    }

    // interpreter
    TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 2);

    interpreter = TfLiteInterpreterCreate(model, options);

    TfLiteModelDelete(model);
    TfLiteInterpreterOptionsDelete(options);

    if (!interpreter) {
        LOGE("Failed to create interpreter.");
        return false;
    }

    // allocating tensors
    if (TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
        LOGE("Failed to allocate tensors.");
        TfLiteInterpreterDelete(interpreter);
        interpreter = nullptr;
        return false;
    }

    // ---
    LOGI("Initialed digit model.");
    return true;
}

bool recognizeDigits(const vector<Mat> &imgs, int digits[]) {
    memset(digits, 0, 81 * sizeof(int));

    if (!interpreter)
    {
        LOGE("Digit model not initialized.");
        return false;
    }
    // ---
    TfLiteTensor *inTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

    int wantedBatchSize = TfLiteTensorDim(inTensor, 0);
    int wantedH =         TfLiteTensorDim(inTensor, 1);
    int wantedW =         TfLiteTensorDim(inTensor, 2);
    int wantedNChannels = TfLiteTensorDim(inTensor, 3);

    if (wantedBatchSize > 0 && wantedBatchSize != 81)
    {
        LOGE("Input batch size must be flexible or 81, nothing else. Current is: %i", wantedBatchSize);
        TfLiteInterpreterDelete(interpreter);
        return false;
    }

    // allocating input buffer
    size_t cellSize = wantedH * wantedW * wantedNChannels;
    inBuff.resize(81 * cellSize);

    // pre-processing
    for (int i = 0; i < 81; i++) {
        Mat inMat = imgs[i];

        resize(inMat, inMat, Size(wantedH, wantedW));
        inMat.convertTo(inMat, CV_32F, 1. / 255.);

        if (inMat.channels() != wantedNChannels) {
            LOGE("False number of channels provided.");
            TfLiteInterpreterDelete(interpreter);
            return false;
        }

        memcpy(inBuff.data() + i * cellSize, inMat.ptr<float>(), cellSize * sizeof(float));
    }

    // copying data into input tensor
    if (TfLiteTensorCopyFromBuffer(inTensor, inBuff.data(), 81 * cellSize * sizeof(float)) != kTfLiteOk) {
        LOGE("Failed to copy data to input tensor.");
        TfLiteInterpreterDelete(interpreter);
        return false;
    }

    //  inference
    if (TfLiteInterpreterInvoke(interpreter) != kTfLiteOk) {
        LOGE("Failed to invoke interpreter");
        TfLiteInterpreterDelete(interpreter);
        return false;
    }

    // getting output
    const TfLiteTensor *outTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
    int outBatchSize = TfLiteTensorDim(outTensor, 0);
    int outN =         TfLiteTensorDim(outTensor, 1);

    if (outN != 9)
    {
        LOGE("Output must contain 9 values.");
        TfLiteInterpreterDelete(interpreter);
        return false;
    }

    vector<float> pred(outBatchSize * outN);

    if (TfLiteTensorCopyToBuffer(outTensor, pred.data(), pred.size() * sizeof(float)) != kTfLiteOk) {
        LOGE("Failed to copy output data from tensor.");
        TfLiteInterpreterDelete(interpreter);
        return false;
    }

    // ---
    for (int i = 0; i < 81; i++)
    {
        auto first = pred.begin() + i * outN;
        auto last = first + outN;

        digits[i] = (int)(max_element(first, last) - first) + 1;
    }

    // ---
    return true;
}

void cleanupDigitModel()
{
    modelBuff.clear();
    if (interpreter)
    {
        TfLiteInterpreterDelete(interpreter);
        interpreter = nullptr;
    }
}

// actual functions
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_sudokusolver_ImageAnalyzer_processFrame(
    JNIEnv */*env*/,
    jobject /*self*/,
    jlong pMat
) {
    // ---
    auto curT = chrono::high_resolution_clock::now();
    auto dt = chrono::duration_cast<chrono::microseconds>(curT - prevT).count() / 1000;

    int fps = (int)(1000 / (float)dt);

    if (dt <= WAIT_TIME)
        return false;

    // ---
    Mat &out = *reinterpret_cast<Mat *>(pMat);
    Mat frame = out.clone();

    int w = out.cols;
    int h = out.rows;
    int area = w * h;

    float cntAreaMin = AREA_MIN_REL * (float) area;

    // --- pre-processing
    cvtColor(frame, frame, COLOR_RGBA2GRAY);
    GaussianBlur(frame, frame, Size(5, 5), 2);
    adaptiveThreshold(frame, frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 11, 2);

    // --- contours
    vector<vector<Point>> contours;
    findContours(frame, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // --- biggest quad
    vector<Point> biggestQuad(4);
    double areaMax = 0;

    for (const auto &contour: contours) {
        double cntArea = contourArea(contour);
        if (cntArea < cntAreaMin)
            continue;

        double arcLen = arcLength(contour, true);

        vector<Point> approx;
        approxPolyDP(contour, approx, 0.02 * arcLen, true);

        if (approx.size() == 4) {
            if (cntArea > areaMax) {
                biggestQuad = approx;
                areaMax = cntArea;
            }
        }
    }

    // --- drawing
    if (areaMax > 0) {
        polylines(
            out,
            biggestQuad,
            true,
            RED,
            2,
            LINE_AA
        );
    }

    putText(
        out,
        (to_string(fps) + " FPS"),
        Point(10, 16),
        FONT_HERSHEY_SIMPLEX,
        0.75,
        RED,
        1,
        LINE_AA
    );

    // ---
    frame.release();
    prevT = chrono::high_resolution_clock::now();

    // ---
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_sudokusolver_CameraFragment_initializeLib(
    JNIEnv *env,
    jobject /*self*/,
    jobject assetMgr
)
{
    AAssetManager *mgr = AAssetManager_fromJava(env, assetMgr);
    AAsset *digitModelAsset = AAssetManager_open(mgr, "digit-model.tflite", AASSET_MODE_BUFFER);
    if (!digitModelAsset) {
        LOGE("Failed to open digit model asset.");
        return false;
    }

    initDigitModel(digitModelAsset);

    AAsset_close(digitModelAsset);

    LOGI("Native library initialized.");
    return true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_sudokusolver_CameraFragment_cleanupLib(
    JNIEnv* /*env*/,
    jobject /*self*/
)
{
    cleanupDigitModel();
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
    if (resClass == nullptr)
        return nullptr;

    jmethodID ctor = env->GetMethodID(
        resClass,
        "<init>",
        "(IJJJJJJ)V"
    );
    if (ctor == nullptr)
        return nullptr;

    // ---
    Mat &out = *reinterpret_cast<Mat *>(pMat);
    Mat frame = out.clone();

    Mat paperOrg(Size(PAPER_S, PAPER_S), out.type());
    Mat paperThresh(Size(PAPER_S, PAPER_S), out.type());
    Mat paper(Size(PAPER_S, PAPER_S), out.type());

    int w = out.cols;
    int h = out.rows;
    int area = w * h;

    float cntAreaMin = AREA_MIN_REL * (float) area;

    auto p2 = chrono::high_resolution_clock::now();

    // --- pre-processing
    cvtColor(frame, frame, COLOR_RGBA2GRAY);
    GaussianBlur(frame, frame, Size(5, 5), 2);
    adaptiveThreshold(frame, frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 11, 2);

    // --- contours
    vector<Cnt> contours;
    findContours(frame, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // --- biggest quad
    Poly biggestQuad(4);
    double areaMax = 0;

    for (const Cnt &cnt: contours) {
        double cntArea = contourArea(cnt);
        if (cntArea < cntAreaMin)
            continue;

        double arcLen = arcLength(cnt, true);

        vector<Point> poly;
        approxPolyDP(cnt, poly, 0.02 * arcLen, true);

        if (poly.size() == 4) {
            if (cntArea > areaMax) {
                biggestQuad = poly;
                areaMax = cntArea;
            }
        }
    }

    if (areaMax == 0)
        return env->NewObject(resClass, ctor, PAPER_NOT_FOUND, 0l, 0l, 0l, 0l, 0l, 0l);

    auto p3 = chrono::high_resolution_clock::now();

    // --- getting paper
    Point2f src[4];
    Point2f dst[4] = {
        {0.f,             0.f},
        {(float) PAPER_S, 0.f},
        {0.f,             (float) PAPER_S},
        {(float) PAPER_S, (float) PAPER_S}
    };

    reorderQuadForWarp(biggestQuad, src);
    Mat warpMat = getPerspectiveTransform(src, dst);

    warpPerspective(out, paperOrg, warpMat, Point(PAPER_S, PAPER_S));

    // --- pre-processing paper
    cvtColor(paperOrg, paper, COLOR_RGBA2GRAY);
    adaptiveThreshold(paper, paper, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 57, 5);

    paper.copyTo(paperThresh);

    // --- processing paper
    // contours
    contours.clear();
    findContours(paper, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);

    // removing small contours
    vector<Cnt> contoursToRemove;

    for (const Cnt &cnt: contours) {
        double cntArea = contourArea(cnt);
        if (cntArea < (double) area / 100) {
            contoursToRemove.push_back(cnt);
        }
    }

    drawContours(paper, contoursToRemove, -1, Scalar(0), -1, LINE_AA);

    // removing (still remaining) speckles
    Mat openingKernel = getStructuringElement(MORPH_RECT, Point(3, 3));
    morphologyEx(paper, paper, MORPH_OPEN, openingKernel);

    // fixing lines
    Mat vertKernal = getStructuringElement(MORPH_RECT, Point(1, 5));
    Mat horiKernal = getStructuringElement(MORPH_RECT, Point(5, 1));

    morphologyEx(paper, paper, MORPH_CLOSE, vertKernal, Point(-1, -1), 4);
    morphologyEx(paper, paper, MORPH_CLOSE, horiKernal, Point(-1, -1), 4);

    // extracting cells
    vector<Poly> cellPolys;
    vector<Mat> cells;
    vector<bool> cellEmtptys;

    paper = 255 - paper;
    paperThresh = 255 - paperThresh;   // TODO: check validity

    contours.clear();
    findContours(paper, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);
    sortContours(contours);

    for (const Cnt &cnt: contours) {
        double arcLen = arcLength(cnt, true);

        Poly approx;
        approxPolyDP(cnt, approx, 0.02 * arcLen, true);

        if (approx.size() == 4)
            cellPolys.push_back(approx);
    }

    if (cellPolys.size() < 81)
        return env->NewObject(resClass, ctor, LESSER_CELLS_FOUND, 0l, 0l, 0l, 0l, 0l, 0l);
    else if (cellPolys.size() > 81)
        return env->NewObject(resClass, ctor, GREATER_CELLS_FOUND, 0l, 0l, 0l, 0l, 0l, 0l);

    Point2f _1[4];
    Point2f _2[4] = {
        {0.f,            0.f},
        {(float) CELL_S, 0.f},
        {0.f,            (float) CELL_S},
        {(float) CELL_S, (float) CELL_S}
    };
    Rect _3;

    for (const auto &cellPoly : cellPolys) {
        Mat cell(CELL_S, CELL_S, paperThresh.type());

        reorderQuadForWarp(cellPoly, _1);
        Mat cellWarpMat = getPerspectiveTransform(_1, _2);

        warpPerspective(paperThresh, cell, cellWarpMat, Point(CELL_S, CELL_S));

        bool isEmpty = isCellEmpty2(cell, _3);

        if (!isEmpty) {
            Rect cropRect(
                max(_3.x - PAD, 0),
                max(_3.y - PAD, 0),
                min(_3.width  + 2*PAD, cell.cols - max(_3.x - PAD, 0)),
                min(_3.height + 2*PAD, cell.rows - max(_3.y - PAD, 0))
            );

            Mat digitCrop = cell(cropRect).clone();

            int top    = max(0, PAD - _3.y);
            int left   = max(0, PAD - _3.x);
            int bottom = max(0, (_3.y + _3.height + PAD) - cell.rows);
            int right  = max(0, (_3.x + _3.width  + PAD) - cell.cols);

            copyMakeBorder(digitCrop, cell, top, bottom, left, right, BORDER_CONSTANT, Scalar(0));
        }

        cells.push_back(cell);
        cellEmtptys.push_back(isEmpty);
    }

    auto p4 = chrono::high_resolution_clock::now();

    // --- digit recognition
    int digits[81];

    recognizeDigits(cells, digits);

    for (int i = 0; i < 81; i++)
    {
        if (cellEmtptys[i])
            digits[i] = 0;
    }

    LOGD("Paper:\n%s", paperToStr(digits, false).c_str());

    if (!isSudokuValid(digits))
        return env->NewObject(resClass, ctor, INVALID_SUDOKU, 0l, 0l, 0l, 0l, 0l, 0l);

    auto p5 = chrono::high_resolution_clock::now();

    // --- solution
    int solution[81];
    memcpy(solution, digits, 81 * sizeof(int));

    bool solved = solve(solution);
    LOGD("Solution:\n%s", paperToStr(solution, false).c_str());

    auto p6 = chrono::high_resolution_clock::now();

    // --- drawing
    for (size_t i = 0; i < cellPolys.size(); i++) {
        const Poly &poly = cellPolys[i];
        const Point centre = getCentre(poly);

        string txt = to_string(cellEmtptys[i] ? solution[i] : digits[i]);

        const Size txtSize = getTextSize(txt, FONT_HERSHEY_SIMPLEX, 3, 2, nullptr);
        const Point org(
            centre.x - txtSize.width / 2,
            centre.y + txtSize.height / 2
        );

        putText(paperOrg, txt, org, FONT_HERSHEY_SIMPLEX, 3, cellEmtptys[i] ? RED : GREEN, 2, LINE_AA);
    }

    Mat warpMatInv;
    invert(warpMat, warpMatInv);

    warpPerspective(paperOrg, out, warpMatInv, out.size(), INTER_CUBIC, BORDER_TRANSPARENT);
//    resize(paperThresh, out, out.size(), 0, 0, INTER_CUBIC);

    auto p7 = chrono::high_resolution_clock::now();

    // --- cleanup
    frame.release();
    paperOrg.release();
    paper.release();
    paperThresh.release();

    auto p8 = chrono::high_resolution_clock::now();

    // --- result
    auto processingImgT = chrono::duration_cast<chrono::milliseconds>(p3 - p2).count();
    auto processingPprT = chrono::duration_cast<chrono::milliseconds>(p4 - p3).count();
    auto digitRecogT =    chrono::duration_cast<chrono::milliseconds>(p5 - p4).count();
    auto solnT =          chrono::duration_cast<chrono::microseconds>(p6 - p5).count();
    auto drawingT =       chrono::duration_cast<chrono::milliseconds>(p7 - p6).count();
    auto totalT =         chrono::duration_cast<chrono::milliseconds>(p8 - p1).count();

    jobject res = env->NewObject(resClass, ctor,
         (solved) ? OK : COULDNT_SOLVE,
         processingImgT,
         processingPprT,
         digitRecogT,
         solnT,
         drawingT,
         totalT
    );

    return res;
}