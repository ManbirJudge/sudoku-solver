# --- imports
import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'
import cv2
import numpy as np
import pyautogui
from tensorflow.keras.models import load_model

from solver import solve, print_paper

# --- constants
AREA_MIN = 50
BORDER_W = 4

PAPER_IMG_S = 1080
CELL_IMG_S = 128

# --- enums
class Color:
    BLUE   = (180, 120, 60)
    GREEN  = (120, 180, 60)
    RED    = (0, 0, 255)
    PURPLE = (160, 130, 200)
    YELLOW = (100, 200, 200)

# --- helper functions
def find_biggest_quad(contours: list) -> np.ndarray | None:
    biggest = None
    max_area = 0

    for cnt in contours:
        # if not cv2.isContourConvex(cnt):
        #     continue

        area = cv2.contourArea(cnt)

        p = cv2.arcLength(cnt, True)
        approx = cv2.approxPolyDP(cnt, 0.02 * p, True)

        if len(approx) == 4 and area > max_area:
            pts = approx.reshape(4, 2)

            sum = pts.sum(axis=1)
            diff = np.diff(pts, axis=1)

            w = np.linalg.norm(pts[np.argmin(sum)] - pts[np.argmin(diff)])
            h = np.linalg.norm(pts[np.argmin(sum)] - pts[np.argmax(diff)])

            if h == 0:
                continue
            
            if 0.5 <= w / h <= 2:
                biggest = approx
                max_area = area

    return biggest

def reoder_quad(quad: np.ndarray) -> np.ndarray:
    quad = quad.reshape((4, 2))
    new_quad = np.zeros((4, 1, 2), dtype=np.int32)

    sum = quad.sum(axis=1)
    diff = np.diff(quad, axis=1)

    new_quad[0] = quad[np.argmin(sum)]
    new_quad[3] = quad[np.argmax(sum)]
    new_quad[1] = quad[np.argmin(diff)]
    new_quad[2] = quad[np.argmax(diff)]

    return new_quad

def is_empty(img, border_size=2, white_threshold=250, fill_ratio=0.3):
    h, w = gray.shape
    cropped = gray[border_size:h-border_size, border_size:w-border_size]

    non_white = np.sum(cropped < white_threshold)
    total_pixels = cropped.size

    return (non_white / total_pixels) < fill_ratio

# --- model
digit_model = load_model('data/digit-model.h5')

# --- image
img = np.array(pyautogui.screenshot())

H, W = img.shape[:2]

paper_img = np.zeros((PAPER_IMG_S, PAPER_IMG_S, 3), dtype=np.uint8)

# --- pre-processing
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
blur = cv2.GaussianBlur(gray, (7, 7), 2)
thresh = cv2.adaptiveThreshold(blur, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 7, 4)

# --- contours
contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
contours = list(filter(lambda x: cv2.contourArea(x) > AREA_MIN, contours))

# --- finding biggest square
biggest_quad = find_biggest_quad(contours)
if biggest_quad is not None:
    biggest_quad = reoder_quad(biggest_quad)

# --- warping
if biggest_quad is not None:
    _1 = np.float32(biggest_quad)
    _2 = np.float32([[0, 0], [W, 0], [0, H], [W, H]])

    warp_mat = cv2.getPerspectiveTransform(_1, _2)

    paper_img = cv2.warpPerspective(gray, warp_mat, (W, H))
    paper_img = cv2.resize(paper_img, (PAPER_IMG_S, PAPER_IMG_S))
    paper_img = cv2.GaussianBlur(paper_img, (5, 5), 2)
    paper_img = cv2.adaptiveThreshold(paper_img, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 9, 2)

# --- getting values
# # splitintg into cells
# if biggest_quad is not None:
#     rows = np.vsplit(paper_img, 9)
#     cells = []

#     for r_idx, row in enumerate(rows):
#         cols = np.hsplit(row, 9)
        
#         for c_idx, cell in enumerate(cols):
#             cell = np.asarray(cell)

#             top_trim    = BORDER_W
#             bottom_trim = BORDER_W
#             left_trim   = BORDER_W
#             right_trim  = BORDER_W

#             EXTRA_W = BORDER_W

#             if r_idx == 0: top_trim += EXTRA_W
#             if r_idx == 8: bottom_trim += EXTRA_W
#             if c_idx == 0: left_trim += EXTRA_W
#             if c_idx == 8: right_trim += EXTRA_W

#             cell = cell[top_trim:cell.shape[0]-bottom_trim, left_trim:cell.shape[1]-right_trim]
#             cell = cv2.resize(cell, (CELL_IMG_S, CELL_IMG_S))
#             cell = cell / 255.0
#             cell = np.expand_dims(cell, axis=-1)
        
#             cells.append(cell)

# # empty cells

# # prediction
# paper_vals = []

# if biggest_quad is not None:
#     cv2.imshow('Cell', np.reshape(cells[0], (CELL_IMG_S, CELL_IMG_S)))

#     predictions = digit_model.predict(np.array(cells), verbose=0)
    
#     for i, pred in enumerate(predictions):
#         n = np.argmax(pred)

#         if is_empty(cells[i]):
#             paper_vals.append(0)
#         else:
#             if pred[n] > 0.5:
#                 paper_vals.append(n)
#             else:
#                 paper_vals.append(0)

# --- solution
# solution = paper_vals.copy()
# if biggest_quad is not None:
#     solve(solution)

# print_paper(paper_vals)
# print('----------')
# print_paper(solution)

# --- drawing
paper_img = cv2.cvtColor(paper_img, cv2.COLOR_GRAY2BGR)

cv2.drawContours(img, contours, -1, Color.GREEN, 1, cv2.LINE_AA)
if biggest_quad is not None:
    cv2.drawContours(img, biggest_quad, -1, Color.RED, 8, cv2.LINE_AA)

# if biggest_quad is not None:
#     for i in range(9):
#         for j in range(9):
#             val = paper_vals[i * 9 + j]

#             if val == 0:
#                 cv2.putText(paper_img, str(solution[i * 9 + j]), (120 * j, 120 * (i + 1)), cv2.FONT_HERSHEY_PLAIN, 2, Color.GREEN, 2, cv2.LINE_AA)
#             else:
#                 cv2.putText(paper_img, str(val), (120 * j, 120 * (i + 1)), cv2.FONT_HERSHEY_PLAIN, 2, Color.BLUE, 2, cv2.LINE_AA)

# --- showing
# cv2.imshow('Image', img)
# cv2.imshow('Gray', gray)
# cv2.imshow('Blur', blur)
# cv2.imshow('Thresh', thresh)
cv2.imshow('Paper', paper_img)

cv2.imwrite('data/s4.png', paper_img)

# ---
cv2.waitKey()
cv2.destroyAllWindows()