#include "opencv2/opencv.hpp" // 引入 OpenCV 函式庫
#include <iostream> // 引入 C++ 輸入輸出流函式庫

using namespace cv; // 使用 OpenCV 命名空間
using namespace std; // 使用標準命名空間

/* 自訂函式 */
void detectAndDisplay(void); // 宣告偵測人臉的函式

/* 全域變數 */
String face_cascade_name = "data/haarcascade_frontalface_alt.xml"; // 正面人臉哈爾階層式分類器的檔案名稱
String eyes_cascade_name = "data/haarcascade_eye_tree_eyeglasses.xml"; // 人眼哈爾階層式分類器的檔案名稱

CascadeClassifier face_cascade; // 建立正面人臉哈爾階層式分類器物件
CascadeClassifier eyes_cascade; // 建立人眼哈爾階層式分類器物件

Mat im; // 輸入影像
int option = 5; // 特效選項
VideoCapture cap1("data/sleepy.mp4"); // 建立人臉視訊物件
VideoCapture cap2("data/explosion.mp4"); // 建立特效影片物件
Point eye_centers[2]; // 雙眼中心的(x,y)位置
Point click_position; // 滑鼠點擊位置
bool play_explosion = false; // 播放explosion標誌
int explosion_frame_count = 0; // explosion影片的幀計數器
bool face_detected = false; // 人臉偵測標誌

// 定義滑鼠反應函式
static void mouse_callback(int event, int x, int y, int, void*) {
    // 當滑鼠按下左鍵，根據點選位置，得到選項 (option) 數值 
    if (event == EVENT_LBUTTONDOWN) {
        if (y > im.rows - 50) { // 若點擊位置在影像下方的選項區域
            if (x < 100) option = 1; // 綠色
            else if (x < 200) option = 2; // 瘦臉 
            else if (x < 300) option = 3; // 馬賽克
            else option = 4; // 其他選項
        }
        else if (face_detected) { // 只有偵測到人臉時才能播放explosion
            // 點擊其他區域顯示眼睛處的圓圈並播放explosion影片
            option = 4;
            click_position = Point(x, y);
            play_explosion = true;
            explosion_frame_count = 0;
            cap2.set(CAP_PROP_POS_FRAMES, 0); // 從頭開始播放explosion.mp4
        }
    }
}

/** 主程式 */
int main(void) {
    if (!cap1.isOpened() || !cap2.isOpened()) { // 若無法開啟人臉視訊或explosion影片，顯示錯誤訊息
        printf("--(!)Error loading video/camera\n");
        waitKey(0);
        return -1;
    }

    // 載入人臉與人眼分類器的參數
    if (!face_cascade.load(face_cascade_name)) {
        printf("--(!)Error loading face cascade\n"); // 若無法載入正面人臉哈爾階層式分類器，顯示錯誤訊息
        waitKey(0);
        return -1;
    }
    if (!eyes_cascade.load(eyes_cascade_name)) {
        printf("--(!)Error loading eyes cascade\n"); // 若無法載入人眼哈爾階層式分類器，顯示錯誤訊息
        waitKey(0);
        return -1;
    }

    namedWindow("window"); // 建立顯示影像的視窗
    setMouseCallback("window", mouse_callback); // 設定滑鼠回調函式

    while (char(waitKey(1)) != 27 && cap1.isOpened()) { // 當按鍵不是 Esc 且人臉視訊物件仍在開啟時，持續執行迴圈
        cap1 >> im; // 抓取視訊的畫面
        if (im.empty()) { // 如果沒有抓到畫面
            printf(" --(!) No captured im -- Break!"); // 顯示錯誤訊息
            break;
        }
        /* 偵測人臉，並顯示特效影像 */
        detectAndDisplay();

        // 如果需要播放explosion影片
        if (play_explosion) {
            Mat explosion_frame;
            cap2 >> explosion_frame;
            if (!explosion_frame.empty()) {
                // 縮小explosion影片
                resize(explosion_frame, explosion_frame, Size(), 0.5, 0.5);

                // 去除黑色背景
                Mat mask;
                inRange(explosion_frame, Scalar(0, 0, 0), Scalar(30, 30, 30), mask);
                bitwise_not(mask, mask);

                // 確保點擊位置顯示explosion影片不超出範圍
                int x = min(max(0, click_position.x - explosion_frame.cols / 2), im.cols - explosion_frame.cols);
                int y = min(max(0, click_position.y - explosion_frame.rows / 2), im.rows - explosion_frame.rows);
                Rect roi(x, y, explosion_frame.cols, explosion_frame.rows);

                Mat im_roi = im(roi);
                explosion_frame.copyTo(im_roi, mask);
            }
            explosion_frame_count++;
            if (explosion_frame_count >= cap2.get(CAP_PROP_FRAME_COUNT)) {
                play_explosion = false;
            }
        }

        /* 顯示影像 */
        imshow("window", im);
    }
    return 0;
}

/** detectAndDisplay 函式內容 */
void detectAndDisplay(void) {
    /** 人臉偵測前處理 */
    vector<Rect> faces; // 儲存人臉 ROI 區域的向量
    Mat im_gray; // 灰階影像物件

    // 彩色影像轉灰階
    cvtColor(im, im_gray, COLOR_BGR2GRAY);
    // 對灰階圖像進行直方圖均衡化，增加對比度
    equalizeHist(im_gray, im_gray);

    // 檢測正面人臉
    face_cascade.detectMultiScale(im_gray, faces, 1.1, 4, 0, Size(80, 80));

    /** 如果有偵測到人臉，執行以下敘述 */
    face_detected = !faces.empty();
    if (face_detected) {
        // 找出最大的人臉 ROI 區域
        Rect faceROI = faces[0];
        for (size_t i = 1; i < faces.size(); ++i) {
            if (faces[i].area() > faceROI.area()) {
                faceROI = faces[i];
            }
        }

        // 稍微擴大人臉 ROI，以確保覆蓋整個人臉
        faceROI.x = max(faceROI.x - 10, 0);
        faceROI.y = max(faceROI.y - 10, 0);
        faceROI.width = min(faceROI.width + 20, im.cols - faceROI.x);
        faceROI.height = min(faceROI.height + 20, im.rows - faceROI.y);

        // 偵測人眼位置
        Mat faceROI_gray = im_gray(faceROI);
        vector<Rect> eyes;
        eyes_cascade.detectMultiScale(faceROI_gray, eyes, 1.1, 2, 0, Size(30, 30));
        if (eyes.size() == 2) {
            eye_centers[0] = Point(faceROI.x + eyes[0].x + eyes[0].width / 2, faceROI.y + eyes[0].y + eyes[0].height / 2);
            eye_centers[1] = Point(faceROI.x + eyes[1].x + eyes[1].width / 2, faceROI.y + eyes[1].y + eyes[1].height / 2);
        }
        else {
            eye_centers[0] = Point(faceROI.x + faceROI.width / 3, faceROI.y + faceROI.height / 3);
            eye_centers[1] = Point(faceROI.x + 2 * faceROI.width / 3, faceROI.y + faceROI.height / 3);
        }

        // 顯示特效選項
        putText(im, "Green", Point(10, im.rows - 20), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 0), 2);
        putText(im, "Green", Point(10, im.rows - 22), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        putText(im, "Slim", Point(110, im.rows - 20), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 0), 2);
        putText(im, "Slim", Point(110, im.rows - 22), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
        putText(im, "Mosaic", Point(210, im.rows - 20), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 0), 2);
        putText(im, "Mosaic", Point(210, im.rows - 22), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);

        // 根據選項處理 ROI 影像
        // 選項1: 綠色
        if (option == 1) {
            Mat im_face = im(faceROI);
            Mat im_hsv, mask;

            // 轉換到HSV色彩空間
            cvtColor(im_face, im_hsv, COLOR_BGR2HSV);

            // 定義皮膚色的範圍
            Scalar lower_skin_color = Scalar(0, 0, 0);
            Scalar upper_skin_color = Scalar(40, 255, 255);

            // 找到皮膚色的範圍，並創建掩碼
            inRange(im_hsv, lower_skin_color, upper_skin_color, mask);

            // 只針對HSV色彩空間的第一個通道進行調整
            for (int i = 0; i < im_hsv.rows; i++) {
                for (int j = 0; j < im_hsv.cols; j++) {
                    if (mask.at<uchar>(i, j) > 0) {
                        im_hsv.at<Vec3b>(i, j)[0] = 60; // 將皮膚色改為綠色
                    }
                }
            }

            // 最終轉回至RGB色彩空間
            cvtColor(im_hsv, im_face, COLOR_HSV2BGR);

            // 將處理後的臉部區域影像複製回原影像
            im_face.copyTo(im(faceROI), mask);
        }

        // 選項2: 瘦臉
        else if (option == 2) {
            Mat im_face = im(faceROI);
            Mat map_x, map_y;
            map_x.create(im_face.size(), CV_32FC1);
            map_y.create(im_face.size(), CV_32FC1);

            float strength = 50.0; // 偏移量倍率，可以根據需要調整
            float center_x = faceROI.x + faceROI.width / 2.0;
            float center_y = faceROI.y + faceROI.height / 2.0;

            for (int i = 0; i < im_face.rows; ++i) {
                for (int j = 0; j < im_face.cols; ++j) {
                    float offset_x = (center_x - abs(j - center_x)) * strength / center_x;
                    map_x.at<float>(i, j) = j + offset_x;
                    map_y.at<float>(i, j) = i;
                }
            }

            remap(im_face, im_face, map_x, map_y, INTER_LINEAR, BORDER_REPLICATE);
        }

        // 選項3: 馬賽克
        else if (option == 3) {
            Mat im_face = im(faceROI);
            Mat small_face;
            resize(im_face, small_face, Size(), 0.1, 0.1, INTER_NEAREST);
            resize(small_face, im_face, im_face.size(), 0, 0, INTER_NEAREST);
        }

        // 選項4: 畫眼睛到滑鼠點擊位置的直線
        else if (option == 4) {
            line(im, eye_centers[0], click_position, Scalar(0, 0, 255), 2); // 左眼到點擊位置
            line(im, eye_centers[1], click_position, Scalar(0, 0, 255), 2); // 右眼到點擊位置
        }

        // 繪製人臉區域矩形框，以及上方的學號
        rectangle(im, faceROI, Scalar(0, 255, 0), 2);
        putText(im, "M11202125", Point(faceROI.x, faceROI.y - 10), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
    }

    /* 顯示影像 */
    imshow("window", im);
}
