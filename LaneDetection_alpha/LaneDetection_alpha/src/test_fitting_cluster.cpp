#include "include/def.h"
#include "include/testcase.h"
#include "include/preprocess.h"
#include "include/detect.h"
#include "include/display.h"
#include "include/predict.h"
#include "include/warning.h"

ULONG TestFittingCluster()
{
    ULONG ret = HCOM_OK;

    /* Ԥ����ģ����� */
    Mat srcFrame;
    Mat srcRoiFrame;
    Mat leftRoiFrame;
    Mat rightRoiFrame;
    Mat leftBindFrame;
    Mat rightBindFrame;
    Mat leftFiltedFrame;
    Mat rightFiltedFrame;
    Mat leftClusterImg = cv::Mat::zeros(HEIGHT, ROI_WIDTH, CV_8UC3);
    Mat rightClusterImg = cv::Mat::zeros(HEIGHT, ROI_WIDTH, CV_8UC3);

    Mat lastDetectLaneImg;
    Mat lastPredictImg;

    /* ���ģ����� */
    LANE resetLane = {Point(0,0), Point(0,0), 0};
    vector<vector<Point> > leftCluster;
    vector<vector<Point> > rightCluster;
    vector<LANE> leftLanes;
    vector<LANE> rightLanes;
    vector<LANE> midLanes;
    ROAD detectRoad;

    /* ����ģ����� */
    LANE leftLastLane;
    LANE rightLastLane;

    vector<Mat> curveCoefficients;
    Mat fitCurveImg;
    vector<Point> fitPoints;
    ROAD cruvePredictedRoad;
    vector<ROAD> historyRoads;

    vector<ROAD> lastRoads;
    ROAD lastRoad;
    DOUBLE lastWidthSum = 0;
    INT predictStep = 200;
    ROAD predictedRoad;
    vector<ROAD> lastPredictRoad;
    ROAD realRoad;
    DOUBLE roadWidth = DEFAULT_ROAD_WIDTH;
    Mat lastImgs; 
    vector<LANE> lastLeftPredictLane;
    vector<LANE> lastRightPredictLane;
    vector<LANE> lastMidPredictLane;
    vector<LANE> lastLeftDetectLane;
    vector<LANE> lastRightDetectLane;
    vector<LANE> lastMidDetectLane;
    INT directDeparture = DIRECT_NONE;

    /* ��������ʼ�� */
    KALMAN_FILTER leftKalman;
    KALMAN_FILTER rightKalman;

    ret = InitKalman(leftKalman);
    ret = InitKalman(rightKalman);

    VideoCapture capture("C:\\Users\\Administrator\\Desktop\\���ڳ����߼��Ļ���ϵͳ¼��\\����\\����·��.mp4");
    VideoWriter writer("output.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, Size(WIDTH, HEIGHT));

    while (capture.read(srcFrame))
    {
        ret = GetRoiFrame(srcFrame, srcRoiFrame, leftRoiFrame, rightRoiFrame);

        /* ��ö�ֵͼ�� */
        ret = ConvertToBind(leftRoiFrame, leftBindFrame);
        //imshow("leftBindFrame", leftBindFrame);
        ret = ConvertToBind(rightRoiFrame, rightBindFrame);
        //imshow("rightBindFrame", rightBindFrame);

        ret = DetectLane(leftBindFrame, leftFiltedFrame, leftCluster, leftLanes);
        //imshow("leftFiltedFrame", leftFiltedFrame); 

        ret = DetectLane(rightBindFrame, rightFiltedFrame, rightCluster, rightLanes);
        ret = ResizeRightLanes(rightLanes, RIGHT_ROI_POS);
        //imshow("rightFiltedFrame", rightFiltedFrame);

        ret = GetDetectedRoad(leftLanes, rightLanes, lastRoads, roadWidth, detectRoad);

        ret = PredictRoadWidth(detectRoad, predictStep, lastRoads, lastWidthSum, roadWidth);

        if (1 < lastRoads.size())
        {
            leftLastLane = lastRoads[lastRoads.size() - 2].leftLane;
            rightLastLane = lastRoads[lastRoads.size() - 2].rightLane;

            ret = PredictLaneOnKalman(leftKalman, detectRoad.leftLane, leftLastLane, predictedRoad.leftLane);

            ret = PredictLaneOnKalman(rightKalman, detectRoad.rightLane, rightLastLane, predictedRoad.rightLane);
        }

        ret = GetRealRoad(detectRoad, predictedRoad, roadWidth, realRoad);

        ret = JudgeWarning(realRoad, lastRoads, directDeparture);

        //ret = DisplayLastHundredLanes(lastDetectLaneImg, detectRoad.leftLane, detectRoad.rightLane, detectRoad.middleLane,
        //    lastLeftDetectLane, lastRightDetectLane, lastMidDetectLane);
        //imshow("lastDetectLaneImg", lastDetectLaneImg);

        ret = DisplayLastHundredLanes(lastPredictImg, predictedRoad.leftLane, predictedRoad.rightLane, predictedRoad.middleLane,
            lastLeftPredictLane, lastRightPredictLane, lastMidPredictLane);
        imshow("kalmanPredictImg", lastPredictImg);

        //ret = DisplayLastHundredLanes(lastPredictImg, realRoad.leftLane, realRoad.rightLane, realRoad.midLane,
        //    lastLeftPredictLane, lastRightPredictLane, lastMidPredictLane);
        //imshow("lastPredictImg", lastPredictImg);

        //ret = DisplayCluster(leftClusterImg, leftCluster);
        //imshow("leftClusterImg", leftClusterImg);
        //ret = DisplayCluster(rightClusterImg, rightCluster);
        //imshow("rightClusterImg", rightClusterImg);

        //ret = DisplayRoad(srcRoiFrame, detectRoad, Scalar(0, 0, 255));
        ret = DisplayRoad(srcRoiFrame, predictedRoad, directDeparture);
        imshow("srcRoiFrame", srcRoiFrame);
        //ret = DisplayFittingPoint(fitCurveImg, historyRoads, curveCoefficients);
        //imshow("fitCurveImg", fitCurveImg);

        writer << srcRoiFrame;

        leftCluster.clear();
        rightCluster.clear();
        leftLanes.clear();
        rightLanes.clear();
        midLanes.clear();
        fitPoints.clear();
        detectRoad.width = 0;
        detectRoad.leftLane = resetLane;
        detectRoad.rightLane = resetLane;
        detectRoad.middleLane = resetLane;

        waitKey(1);

    }

    return ret;
}