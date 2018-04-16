#include "include/def.h"
#include "include/predict.h"
#include "include/detect.h"

ULONG PredictRoadWidth(ROAD& detectRoad, INT lastRoadNum, vector<ROAD>& lastRoads, DOUBLE& lastWidthSum, DOUBLE& predictWidth);
ULONG PredictKalman(KALMAN_FILTER& kalmanFilter, LANE& detectLane, LANE& predictLane);
ULONG ReformRoadLane(ROAD& road, DOUBLE width);

ULONG GetRealRoad(LANE& detectLane, LANE& predictLane, LANE& realLane);
ULONG GetCubicFittingFactors(vector<Point>& controlPoint, FLOAT* ai, FLOAT* bi, FLOAT* ci, FLOAT* di);
ULONG CalcTDMA(FLOAT* X, ULONG n, FLOAT* A, FLOAT* B, FLOAT* C, FLOAT* D);
ULONG JudgeIsChangeRoad(LANE& middleLane, LANE& lastLane, INT& direct);
ULONG GetNearestNoZeroPoint(vector<Point>& historyRoads, INT& pos, Point& noZeroPoint);
ULONG CalLSMFitFactor(vector<Point>& controlPoint, INT step, Mat& curveCoefficient);
ULONG SampleControlPoint(vector<Point>& historyRoads, INT& controlPointNum, vector<Point>& controlPoints);
ULONG GetCurveLSM(vector<Point>& historyPoints, Mat& curveCoefficient);
ULONG GetRealRoad(Point& detectPoint, Mat& curveCoefficient, Point& realPoint);
ULONG UpdataHistoryRoad(ROAD& presentRoad, INT& historyLength, vector<ROAD>& historyRoads);
ULONG UpdataHistoryPoint(Point& presentPoint, INT& historyLength, vector<Point>& historyPoints);
ULONG DeletePointNotOnCurve(vector<ROAD>& historyRoads, vector<Mat>& curveCoefficient);
ULONG GetCurveRANSAC(vector<Point>& historyPoints, Mat& curveCoefficients);
ULONG SamplePointRandom(vector<Point>& fitPoints, INT& samplePointNum, vector<Point>& samplePoints, vector<Point>& remainingPoints);
ULONG CalQuadPolyFactor(vector<Point>& samplePoints, Mat& curveCoefficients);
ULONG ConductRansac(Mat& currentCurveCoefficients, vector<Point> remainingPoint, Mat& curveCoefficients, INT& bestError);

ULONG InitKalman(KALMAN_FILTER& kalmanFilter)
{
    ULONG ret = HCOM_OK;

     KalmanFilter kf(KALMAN_STATE_NUM, KALMAN_MEASUREMENT_NUM, 0);
     kalmanFilter.kf = kf;

    Mat state (KALMAN_STATE_NUM, 1, CV_32FC1);
    Mat processNoise(KALMAN_STATE_NUM, 1, CV_32F);
    Mat measurement = Mat::zeros(KALMAN_MEASUREMENT_NUM, 1, CV_32F);
    kalmanFilter.measure = measurement;

    randn( state, Scalar::all(0), Scalar::all(0.1) );
    kalmanFilter.kf.transitionMatrix = *(Mat_<FLOAT>(KALMAN_STATE_NUM, KALMAN_STATE_NUM) << 
        1, 0, 0.8, 0, 
        0, 1, 0, 0.5,
        0, 0, 0.8, 0,
        0, 0, 0, 0.5);

    // ��������
    setIdentity(kalmanFilter.kf.measurementMatrix);

    // ��������Э�������
    setIdentity(kalmanFilter.kf.processNoiseCov, Scalar::all(1e-4));

    // �۲�����Э�������
    setIdentity(kalmanFilter.kf.measurementNoiseCov, Scalar::all(1e-1));

    // �������Э�������
    setIdentity(kalmanFilter.kf.errorCovPost, Scalar::all(5));

    // X��0����ʼ��
    randn(kalmanFilter.kf.statePost, Scalar::all(0), Scalar::all(0.1));

    randn(state, Scalar::all(0), Scalar::all(0));

    return ret;
}

ULONG PredictKalman(KALMAN_FILTER& kalmanFilter, LANE& detectLane, LANE& predictLane)
{
    ULONG ret = HCOM_OK;
    LANE optimizeLane;

    //Ԥ��
    Mat prediction = kalmanFilter.kf.predict();
    predictLane.s = prediction.at<FLOAT>(0);
    predictLane.theta = prediction.at<FLOAT>(1);

    //���²���
    kalmanFilter.measure.at<FLOAT>(0) = detectLane.s;
    kalmanFilter.measure.at<FLOAT>(1) = detectLane.theta;

    //����
    kalmanFilter.kf.correct( kalmanFilter.measure);

    optimizeLane.s = kalmanFilter.kf.statePost.at<FLOAT>(0);
    optimizeLane.theta = kalmanFilter.kf.statePost.at<FLOAT>(1);

    predictLane = optimizeLane;
    return ret;
}

ULONG PredictLaneOnKalman(KALMAN_FILTER& kalmanFilter, LANE& detectLane, LANE& lastLane, LANE& predictLane)
{
    ULONG ret = HCOM_OK;
    INT direct = DIRECT_NONE;
 
    /* δ��⵽ */
    if (0 == detectLane.length)
    {
        predictLane = detectLane;
        return ret;
    }
    else
    {
        ret = JudgeIsChangeRoad(detectLane, lastLane, direct);
        if (DIRECT_NONE != direct)
        {
            ret = InitKalman(kalmanFilter);
        }

        /* ���뿨����Ԥ���� */
        ret = PredictKalman(kalmanFilter, detectLane,  predictLane);

        /* ת��Ϊֱ������ϵ */
        ret = CvtLaneToRectCoordinate(predictLane);
    }

    return ret;
}


/*****************************************************************************
  �� �� ��  : PredictRoadWidth
  ��������  : Ԥ���·���
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��19��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG PredictRoadWidth(ROAD& detectRoad, INT& predictStep, vector<ROAD>& lastRoads, DOUBLE& lastWidthSum, DOUBLE& roadWidth)
{
    ULONG ret = HCOM_OK;
    
    /* δ��⵽ */
    if (detectRoad.width < MIN_ROAD_WIDTH)
    {
        return ret;
    }
    
    /* ��ƽ���ķ�ʽԤ���� */
    if (lastRoads.size() < predictStep)
    {
        lastWidthSum += detectRoad.width;
        roadWidth = lastWidthSum / (lastRoads.size() + 1);        
    }
    else
    {
        lastWidthSum = lastWidthSum - lastRoads[0].width + detectRoad.width;
        roadWidth = lastWidthSum / (lastRoads.size() + 1);
    }
    
    /* ���´洢��·���ݵ����� */
    ret = UpdataHistoryRoad(detectRoad, predictStep, lastRoads);

    return ret;
}

ULONG ReformRoadLane(ROAD& road, DOUBLE width)
{
    ULONG ret = HCOM_OK;

    width /= 2;
    road.leftLane.headPoint = Point(road.middleLane.headPoint.x - width, 0);
    road.leftLane.backPoint = Point(road.middleLane.backPoint.x - width, HEIGHT);

    road.rightLane.headPoint = Point(road.middleLane.headPoint.x + width, 0);
    road.rightLane.backPoint = Point(road.middleLane.backPoint.x + width, HEIGHT);

    return ret;
}

ULONG RecordLastRoad(ROAD& detectRoad, ROAD& lastRoad)
{
    ULONG ret = HCOM_OK;

    /* ֻ��¼������0�� */
    if (0 != detectRoad.leftLane.headPoint.x)
    {
        lastRoad.leftLane = detectRoad.leftLane;
    }
    if (0 != detectRoad.rightLane.headPoint.x)
    {
        lastRoad.rightLane = detectRoad.rightLane;
    }

    return ret;
}

/* �ж��Ƿ������� */
ULONG JudgeIsChangeRoad(LANE& lane, LANE& lastLane, INT& direct)
{
    ULONG ret = HCOM_OK;
    DOUBLE widthError = 100;
    DOUBLE headOffset = 0;
    DOUBLE backOffset = 0;
    direct = DIRECT_NONE;

    /* ��β�����Ҫ���� */
    headOffset = lane.headPoint.x - lastLane.headPoint.x;
    backOffset = lane.backPoint.x - lastLane.backPoint.x;

    /* λ�÷���ͻ�䣬�ұ仯��СԼΪ��·��� */
    if (abs(headOffset) > widthError
        && abs(backOffset) > widthError
        && (headOffset * backOffset) > 0)
    {
        /* ����ת�� */
        direct =  headOffset > 0 ? DIRECT_RIGHT : DIRECT_LEFT;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : GetRealRoad
  ��������  : ����Ԥ����ʵ�ʻ����ʵֵ
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��19��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetRealRoad(ROAD& detectRoad, ROAD& predictRoad, DOUBLE& roadWidth, ROAD& realRoad)
{
    ULONG ret = HCOM_OK;
 
    INT residuals = 0;      //�в�
    INT error = 10;

    /* �����·������ */
    ret = GetMidLanes(predictRoad);

    ret = CvtLaneToPolarCoodinate(predictRoad.middleLane);

    realRoad = predictRoad;

    return ret;
}

/* �����������Ԥ�� */
ULONG PredictCubic(ROAD& detectRoad, vector<ROAD>& lastFitRoad, ROAD& predictRoad, vector<Point>& fittingPoint)
{
    ULONG ret = HCOM_OK;
    INT fittingNum = 200;
    INT controlPointNum = 5;
    INT controlPointPos[6];
    vector<Point> controlPoint;
    Point noZeroPoint;
    INT fittedPos = 0;
    FLOAT x = 0;
    FLOAT y = 0;

    controlPointPos[0] = 0;
    for (INT i = 1; i < controlPointNum; i++)
    {
        controlPointPos[i] = controlPointPos[i - 1]  + fittingNum / controlPointNum;
    }
    controlPointPos[5] = 199;

    /* ����ϵ���ڴ���� */
    FLOAT* ai = (FLOAT*)malloc(sizeof(FLOAT) * (controlPointNum));
    CHECK_PTR_IS_NULL(ai);
    FLOAT* bi = (FLOAT*)malloc(sizeof(FLOAT) * (controlPointNum));
    if (NULL_PTR == bi)
    {
        free(ai);
        return ERROR_PTR_NULL;
    }
    FLOAT* ci = (FLOAT*)malloc(sizeof(FLOAT) * (controlPointNum));
    if (NULL_PTR == ci)
    {
        free(ai);
        free(bi);
        return ERROR_PTR_NULL;
    }
    FLOAT* di = (FLOAT*)malloc(sizeof(FLOAT) * (controlPointNum));  
    if (NULL_PTR == di)
    {
        free(ai);
        free(bi);
        free(ci);
        return ERROR_PTR_NULL;
    }
    FLOAT* h = (FLOAT*)malloc(sizeof(FLOAT) * (controlPointNum));
    if (NULL_PTR == h)
    {
        free(ai);
        free(bi);
        free(ci);
        free(di);
        return ERROR_PTR_NULL;
    }

    /* ����ϵĵ� */
    if (lastFitRoad.size() < fittingNum)
    {
        lastFitRoad.push_back(detectRoad);
    }
    else
    {
        lastFitRoad.erase(lastFitRoad.begin());
        lastFitRoad.push_back(detectRoad);
    }

    /* ����󳵵����׵� */
    if (fittingNum == lastFitRoad.size())
    {
        for (INT i = 0; i <= controlPointNum; i++)
        {            
            //ret = GetNearestNoZeroPoint(lastFitRoad, controlPointPos[i], noZeroPoint);
            controlPoint.push_back(noZeroPoint);
        }

        ret = GetCubicFittingFactors(controlPoint, ai, bi, ci, di);
        
        for (INT i = 0; i < controlPointNum; i++)
        {
            h[i] = controlPoint[i + 1].x - controlPoint[i].x;
        }

        for (INT i = 0; i < controlPointNum; i++)
        {
            for (INT j = 0; j < h[i]; j++)
            {
                x = controlPoint[i].x + j;
                y = ai[i] 
                    + bi[i] * (x - controlPoint[i].x) 
                    + ci[i] * (x - controlPoint[i].x) * (x - controlPoint[i].x) 
                    + di[i] * (x - controlPoint[i].x) * (x - controlPoint[i].x) * (x - controlPoint[i].x);
                fittingPoint.push_back(Point(x, y));
            }
        }

        //fittedPos = ai[controlPointNum - 1] 
        //            + bi[controlPointNum - 1]  * (fittingNum - controlPoint.back().x) 
        //            + ci[controlPointNum - 1]  * (fittingNum - controlPoint.back().x) * (fittingNum - controlPoint.back().x) 
        //            + di[controlPointNum - 1]  * (fittingNum - controlPoint.back().x) * (fittingNum - controlPoint.back().x) * (fittingNum - controlPoint.back().x);

        //predictRoad.leftLane.headPoint = Point(fittedPos, 0);
    }

    controlPoint.clear();
    free(ai);
    free(bi);
    free(ci);
    free(di);

    return ret;
}

ULONG GetCubicFittingFactors(vector<Point>& controlPoint, FLOAT* ai, FLOAT* bi, FLOAT* ci, FLOAT* di)
{
    ULONG ret = HCOM_OK;
    ULONG i = 0;
    ULONG j = 0;
    ULONG pointNum = controlPoint.size();
    FLOAT x = 0;
    FLOAT y = 0;

    if (pointNum < 3)
    {
        return ret;
    }

    FLOAT* h = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum-1));
    if (NULL_PTR == h)
    {
        return ERROR_PTR_NULL;
    }

    FLOAT* A = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum-2));
    CHECK_PTR_IS_NULL(A);
    FLOAT* B = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum-2));
    if (NULL_PTR == B)
    {
        free(A);
        return ERROR_PTR_NULL;
    }
    FLOAT* C = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum-2));
    if (NULL_PTR == C)
    {
        free(A);
        free(B);
        return ERROR_PTR_NULL;
    }
    FLOAT* D = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum-2));  
    if (NULL_PTR == D)
    {
        free(A);
        free(B);
        free(C);
        return ERROR_PTR_NULL;
    }
    FLOAT* E = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum-2));
    if (NULL_PTR == E)
    {
        free(A);
        free(B);
        free(C);
        free(D);
        return ERROR_PTR_NULL;
    }
    FLOAT* M = (FLOAT*)malloc(sizeof(FLOAT) * (pointNum));
    if (NULL_PTR == M)
    {
        free(A);
        free(B);
        free(C);
        free(D);
        free(E);
        return ERROR_PTR_NULL;
    }

    //����x�Ĳ��� pointNum -1 ������
    for ( i = 0; i < pointNum - 1; i++)
    {
        h[i] = controlPoint[i + 1].x - controlPoint[i].x;
    }

    /* M�����ϵ��
     *[1,...
     *[A1, B1, C1, ...
     *[0,  A2, B2, C2, ...
     *[0, ...             An-1, Bn-1��Cn-1
                                ...,   1 ]
     */
    for( i = 0; i < pointNum - 2; i++)
    {
        A[i] = h[i];                    //0, 1�����
        B[i] = 2 * (h[i] + h[i+1]);
        C[i] = h[i+1];                  
    }
   
    for (i = 0; i < pointNum - 2; i++)
    {
        D[i] = 6 * ((controlPoint[i + 2].y - controlPoint[i + 1].y) / h[i + 1] 
            - (controlPoint[i + 1].y - controlPoint[i].y) / h[i]);
    }
    
    
    //������ԽǾ���
    ret = CalcTDMA(E, pointNum - 2, A, B, C, D);
    
    M[0] = 0; //��Ȼ�߽���׶�MΪ0
    M[pointNum-1] = 0;  //��Ȼ�߽��ĩ��MΪ0
    for(i=1; i< pointNum - 1; i++)
    {
        M[i] = E[i-1]; //������Mֵ
    }
    
    //���������������ߵ�ϵ��
    for( i = 0; i < pointNum - 1; i++)
    {
        ai[i] = controlPoint[i].y;
        bi[i] = (controlPoint[i + 1].y - controlPoint[i].y) / h[i] - (2 * h[i] * M[i] + h[i] * M[i + 1]) / 6;
        ci[i] = M[i] / 2;
        di[i] = (M[i + 1] - M[i]) / (6 * h[i]);
    }

    free(h);
    free(A);
    free(B);
    free(C);
    free(D);
    free(E);
    free(M);
    return ret;
}

/* �������ԽǾ��� */
ULONG CalcTDMA(FLOAT* X, ULONG n, FLOAT* A, FLOAT* B, FLOAT* C, FLOAT* D)
{
    ULONG ret = HCOM_OK;

    INT i;
    FLOAT tmp;

  //�����Ǿ���
    if (0 != B[0])
    {
        C[0] = C[0] / B[0];
        D[0] = D[0] / B[0];
    }

    for(i = 1; i < n; i++)
    {
        tmp = B[i] - A[i] * C[i-1];
        if(0 != tmp)
        {
            C[i] = C[i] / tmp;
            D[i] = (D[i] - A[i] * D[i-1]) / tmp;
        }
    }

    //���һ��ֵ
    X[n-1] = D[n-1];

    //��������� ���X
    for(i = n - 2; i >= 0; i--)
    {
        X[i] = D[i] - C[i] * X[i+1];
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : UpdataHistoryRoad
  ��������  : ������ʷ��·����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��19��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG UpdataHistoryRoad(ROAD& presentRoad, INT& historyLength, vector<ROAD>& historyRoads)
{
    ULONG ret = HCOM_OK;

    if (historyRoads.size() < historyLength)
    {
        historyRoads.push_back(presentRoad);
    }
    else
    {
        historyRoads.erase(historyRoads.begin());
        historyRoads.push_back(presentRoad);
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : UpdataHistoryPoint
  ��������  : ������ʷ��·����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��19��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG UpdataHistoryPoint(Point& presentPoint, INT& historyLength, vector<Point>& historyPoints)
{
    ULONG ret = HCOM_OK;

    /* δ��⵽Ĭ��Ϊ0 ���������*/
    if (0 == presentPoint.x)
    {
        return ret;
    }

    if (historyPoints.size() < historyLength)
    {
        historyPoints.push_back(presentPoint);
    }
    else
    {
        historyPoints.erase(historyPoints.begin());
        historyPoints.push_back(presentPoint);
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : PredictCurve
  ��������  : ͨ���������Ԥ��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��19��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG PredictCurve(ROAD& detectRoad, vector<ROAD>& historyRoads, vector<Mat>& curveCoefficients, ROAD& realRoad)
{
    ULONG ret = HCOM_OK;

    INT historyLength = FIT_NUM;
    //Mat leftHeadCurveCoefficient;
    //Mat leftBackCurveCoefficient;
    //Mat rightHeadCurveCoefficient;
    //Mat rightBackCurveCoefficient;
    Mat midHeadCurveCoefficient;

    //vector<Point> leftHeadHistoryPoints;
    //vector<Point> leftBackHistoryPoints;
    //vector<Point> rightHeadHistoryPoints;
    //vector<Point> rightBackHistoryPoints;
    vector<Point> midHeadHistoryPoints;

    /* ������϶��� */
    ret = UpdataHistoryRoad(detectRoad, historyLength, historyRoads);

    if (FIT_NUM == historyRoads.size())
    {       
        //for (INT i = 0; i < FIT_NUM; i++)
        //{
        //    leftHeadHistoryPoints.push_back(historyRoads[i].leftLane.headPoint);
        //}

        //for (INT i = 0; i < FIT_NUM; i++)
        //{
        //    rightHeadHistoryPoints.push_back(historyRoads[i].rightLane.headPoint);
        //}

        for (INT i = 0; i < FIT_NUM; i++)
        {
            midHeadHistoryPoints.push_back(historyRoads[i].middleLane.headPoint);
        }

        //ret = GetCurveLSM(leftHeadHistoryPoints, leftHeadCurveCoefficient);

        //ret = GetCurveLSM(rightHeadHistoryPoints, rightHeadCurveCoefficient);

        ret = GetCurveLSM(midHeadHistoryPoints, midHeadCurveCoefficient);

        //ret = GetCurveRANSAC(leftHeadHistoryRoads, leftHeadCurveCoefficient);

        //ret = GetCurveRANSAC(rightHeadHistoryRoads, rightHeadCurveCoefficient);

        /* Ԥ�⣬���õ�ʵ��ֵ */
        //ret = GetRealRoad(detectRoad.leftLane.headPoint, leftHeadCurveCoefficient, realRoad.leftLane.headPoint);

        //ret = GetRealRoad(detectRoad.rightLane.headPoint, rightHeadCurveCoefficient, realRoad.rightLane.headPoint);

        ret = GetRealRoad(detectRoad.middleLane.headPoint, midHeadCurveCoefficient, realRoad.middleLane.headPoint);

        curveCoefficients.clear();
        //curveCoefficients.push_back(leftHeadCurveCoefficient);
        //curveCoefficients.push_back(rightHeadCurveCoefficient);
        curveCoefficients.push_back(midHeadCurveCoefficient);

        /* ɾ�����������ϵĵ㣬����� */
        //ret = DeletePointNotOnCurve(historyRoads, curveCoefficients);

    }

    //leftHeadHistoryPoints.clear();
    //leftBackHistoryPoints.clear();
    //rightHeadHistoryPoints.clear();
    //rightBackHistoryPoints.clear();
    midHeadHistoryPoints.clear();

    return ret;
}

/*****************************************************************************
  �� �� ��  : GetFitPoints
  ��������  : ��ȡ��ϵ�Ķ���
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetFitPoints(vector<ROAD>& historyRoads, vector<Point>& fitPoints)
{
    ULONG ret = HCOM_OK;

    return ret;
}

/*****************************************************************************
  �� �� ��  : GetCurveRANSAC
  ��������  : �������һ�³������
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetCurveRANSAC(vector<Point>& historyPoints, Mat& curveCoefficient)
{
    ULONG ret = HCOM_OK;
    vector<Point> samplePoints;
    vector<Point> remainingPoints;
    Mat currentCurveCoefficient;
    INT bestError = 0;
    ULONG iterations = 50;
    INT samplePointNum = 10;

    for (INT i = 0; i < iterations; i++)
    {
        ret = SamplePointRandom(historyPoints, samplePointNum, samplePoints, remainingPoints);
        //ret = CalQuadPolyFactor(samplePoints, currentCurveCoefficients);
        ret = CalLSMFitFactor(samplePoints, 3, currentCurveCoefficient);
        ret = ConductRansac(currentCurveCoefficient, remainingPoints, curveCoefficient, bestError);

        remainingPoints.clear();
        samplePoints.clear();
    }

    if (0 == curveCoefficient.data)
    {
        curveCoefficient = currentCurveCoefficient;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : CalQuadPolyFactor
  ��������  : ������ζ���ʽ���ϵ��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG CalQuadPolyFactor(vector<Point>& samplePoints, Mat& curveCoefficients)
{
    ULONG ret = HCOM_OK;

    curveCoefficients = cv::Mat::zeros(samplePoints.size(), samplePoints.size(), CV_32F);
    Mat x = cv::Mat::zeros(samplePoints.size(), samplePoints.size(), CV_32F);
    Mat xInvert = cv::Mat::zeros(samplePoints.size(), samplePoints.size(), CV_32F);
    Mat y = cv::Mat::zeros(samplePoints.size(), 1, CV_32F);
    
    FLOAT* xData = (FLOAT*)x.data;
    for (INT i = 0; i < x.cols; i++)
    {
        for (INT j = 0; j < x.rows; j++)
        {
            xData[i * x.cols  + j] = pow(samplePoints[i].x, x.cols - j - 1); 
        }
    }
    xInvert = invert(x, xInvert, DECOMP_SVD);
    
    FLOAT* yData = (FLOAT*)y.data;
    for (INT i = 0; i < y.rows; i++)
    {
        yData[i] = samplePoints[i].y;
    }

    curveCoefficients = xInvert * y;

    return ret;
}

/*****************************************************************************
  �� �� ��  : SamplePointRandom
  ��������  : ���������
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG SamplePointRandom(vector<Point>& fitPoints, INT& samplePointNum, vector<Point>& samplePoints, vector<Point>& remainingPoints)
{
    ULONG ret = HCOM_OK;

    ULONG sampleFlag = 0;
    ULONG randomPos = 0;
    Point noZeroPoint;

    INT* sampleIndex = NULL;
    sampleIndex = (INT*)malloc(samplePointNum * sizeof(INT));
    if (NULL_PTR == sampleIndex)
    {
        return ERROR_PTR_NULL;
    }
    memset(sampleIndex, 0, samplePointNum * sizeof(INT));

    /* ������� */
    INT sampleStep = fitPoints.size() / (samplePointNum + 2);
    for (INT i = 1; i < samplePointNum - 1; i++)
    {
        randomPos = rand() % (sampleStep) + i * sampleStep;
        sampleIndex[i] = randomPos;
    }
    sampleIndex[samplePointNum - 1] = fitPoints.size() - 2;

    /* �޳�Ϊ0�� */
    for (INT i = 0; i < samplePointNum; i++)
    {
        ret = GetNearestNoZeroPoint(fitPoints, sampleIndex[i], noZeroPoint);
        samplePoints.push_back(noZeroPoint);
    }

    /* ʣ�µĵ� */
    for (INT i = 0; i < fitPoints.size(); i++)
    {
        sampleFlag = 0;
        for (INT j = 0; j < samplePointNum; j++)
        {         
            if (i == sampleIndex[j])
            {
                sampleFlag = 1;     /* ��ǰi��Ӧ�ĵ㱻������ */             
            }
        }

        if (0 == sampleFlag)
        {
            remainingPoints.push_back(Point(i, fitPoints[i].x));
        }
    }

    free(sampleIndex);
    return ret;

}

/*****************************************************************************
  �� �� ��  : ConductRansac
  ��������  : ransac
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG ConductRansac(Mat& currentCurveCoefficient, vector<Point> remainingPoint, Mat& bestCurveCoefficient, INT& bestError)
{
    ULONG ret = HCOM_OK;

    FLOAT currentY = 0;

    INT errorOffset = 0;
    INT errorThreshold = 2;      /* �������Ƿ���Ӧ�ڵ�ǰģ�͵ķ�ֵ */
    INT consenThreshold = 0;      /* �ж�ģ���Ƿ������������㼯��������Ŀ */

    FLOAT thisError = 0;
    Mat betterCurveCoefficients;

    vector<Point> consenPoint;

    /* �ҳ����·��ϵ�ǰģ�͵ĵ� */
    for (INT i = 0; i < remainingPoint.size(); i++)
    {
        currentY = currentCurveCoefficient.at<DOUBLE>(0,0) 
            + currentCurveCoefficient.at<DOUBLE>(1,0) * remainingPoint[i].x
            + currentCurveCoefficient.at<DOUBLE>(2,0) * std::pow(remainingPoint[i].x, 2)
            + currentCurveCoefficient.at<DOUBLE>(3, 0)*std::pow(remainingPoint[i].x, 3);
       
        //�������ֱ���ϻ򸽽�
        errorOffset = abs(remainingPoint[i].y - currentY);       
        if( errorOffset < errorThreshold)
        {
            consenPoint.push_back(remainingPoint[i]);
        }
    }

    /* �����ϵ�ǰģ�͵ĵ㳬����ֵ����Ϊ��ģ�Ϳ��У������Ѿ����������ģ�ͱȽ� */
    //consenThreshold = remainingPoint.size() / 2;
   // if(consenPoint.size() > consenThreshold)
    //{
                
        //betterCurveCoefficients = currentCurveCoefficients;
        thisError = consenPoint.size();

        /* ��ǰģ�͸��� */
        if (thisError > bestError)
        {
            bestCurveCoefficient = currentCurveCoefficient;
            bestError = thisError;
        }
       

   // }

    consenPoint.clear();
    return ret;
}

/*****************************************************************************
  �� �� ��  : DeletePointNotOnCurve
  ��������  : ɾ�������в��������ϵĵ㣬�����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG DeletePointNotOnCurve(vector<ROAD>& historyRoads, vector<Mat>& curveCoefficient)
{
    ULONG ret = HCOM_OK;
    INT errorPointOnCurve = 5;
    FLOAT y = 0;
    Mat leftHeadCurveCoefficient = curveCoefficient[0];
    Mat leftBackCurveCoefficient;
    Mat rightHeadCurveCoefficient = curveCoefficient[1];
    Mat rightBackCurveCoefficient;

    for (INT i = 0; i < historyRoads.size(); i++)
    {
        y = leftHeadCurveCoefficient.at<DOUBLE>(0, 0) + leftHeadCurveCoefficient.at<DOUBLE>(1, 0) * i +  
            leftHeadCurveCoefficient.at<DOUBLE>(2, 0)*std::pow(i, 2) + leftHeadCurveCoefficient.at<DOUBLE>(3, 0)*std::pow(i, 3);
        if (abs(y - historyRoads[i].leftLane.headPoint.x) > errorPointOnCurve)
        {
            historyRoads[i].leftLane.headPoint = Point(0,0);
        }

        y = rightHeadCurveCoefficient.at<DOUBLE>(0, 0) + rightHeadCurveCoefficient.at<DOUBLE>(1, 0) * i +  
            rightHeadCurveCoefficient.at<DOUBLE>(2, 0)*std::pow(i, 2) + rightHeadCurveCoefficient.at<DOUBLE>(3, 0)*std::pow(i, 3);
        if (abs(y - historyRoads[i].rightLane.headPoint.x) > errorPointOnCurve)
        {
            historyRoads[i].rightLane.headPoint = Point(0,0);
        }
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : GetCurveLSM
  ��������  : ��ȡ��ʷ���ݵ��������
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetCurveLSM(vector<Point>& historyPoints, Mat& curveCoefficient)
{
    ULONG ret = HCOM_OK;
    INT controlPointNum = 20;
    vector<Point> controlPoints;
    FLOAT y = 0;

    if (FIT_NUM != historyPoints.size())
    {
        return HCOM_OK;
    }

    /* �������Ƶ� */
    ret = SampleControlPoint(historyPoints, controlPointNum, controlPoints);
        
    /* ��С������� */
    ret = CalLSMFitFactor(controlPoints, 3, curveCoefficient);
   
    controlPoints.clear();

    return ret;
}

/*****************************************************************************
  �� �� ��  : CalLSMFitFactor
  ��������  : ������С���ˣ���������ϵ��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG CalLSMFitFactor(vector<Point>& controlPoint, INT step, Mat& curveCoefficient)
{
    ULONG ret = HCOM_OK;
    
    INT keyNum = controlPoint.size();

    //�������X
    cv::Mat X = cv::Mat::zeros(step + 1, step + 1, CV_64FC1);
    for (INT i = 0; i < step + 1; i++)
    {
        for (INT j = 0; j < step + 1; j++)
        {
            for (INT k = 0; k < keyNum; k++)
            {
                X.at<DOUBLE>(i, j) = X.at<DOUBLE>(i, j) +
                    std::pow(controlPoint[k].x, i + j);
            }
        }
    }

    //�������Y
    cv::Mat Y = cv::Mat::zeros(step + 1, 1, CV_64FC1);
    for (INT i = 0; i < step + 1; i++)
    {
        for (INT k = 0; k < keyNum; k++)
        {
            Y.at<DOUBLE>(i, 0) = Y.at<DOUBLE>(i, 0) +
                std::pow(controlPoint[k].x, i) * controlPoint[k].y;
        }
    }

    curveCoefficient = cv::Mat::zeros(step + 1, 1, CV_64FC1);

    //������A
    cv::solve(X, Y, curveCoefficient, cv::DECOMP_LU);


    return ret;

}

/*****************************************************************************
  �� �� ��  : SampleControlPoint
  ��������  : �������Ƶ�
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG SampleControlPoint(vector<Point>& historyRoads, INT& controlPointNum, vector<Point>& controlPoints)
{
    ULONG ret = HCOM_OK;
    Point noZeroPoint;

    INT* controlPointPos = NULL;
    controlPointPos = (INT*)malloc(controlPointNum * sizeof(INT));
    if (NULL_PTR == controlPointPos)
    {
        return ERROR_PTR_NULL;
    }
    memset(controlPointPos, 0, controlPointNum * sizeof(INT));

    /* ���ȳ��� */
    for (INT i = 1; i < controlPointNum - 1; i++)
    {
        controlPointPos[i] = controlPointPos[i - 1]  + FIT_NUM / (controlPointNum - 1);
    }
    controlPointPos[controlPointNum - 1] = FIT_NUM - 2;

    /* �޳�Ϊ0�� */
    for (INT i = 0; i < controlPointNum; i++)
    {
        ret = GetNearestNoZeroPoint(historyRoads, controlPointPos[i], noZeroPoint);
        controlPoints.push_back(noZeroPoint);
    }

    free(controlPointPos);
    return ret;
}

/*****************************************************************************
  �� �� ��  : GetNearestNoZeroPoint
  ��������  : �ҵ������㸽����Ϊ0�ĵ�
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetNearestNoZeroPoint(vector<Point>& historyRoads, INT& pos, Point& noZeroPoint)
{
    ULONG ret = HCOM_OK;
    INT i = 0;

    if (pos > historyRoads.size())
    {
        return ret;
    }

    if (0 != historyRoads.at(pos).x)
    {
        noZeroPoint = Point(pos, historyRoads.at(pos).x);
    }
    else
    {
        i = pos;
        while (0 == historyRoads.at(i).x)
        {
            /* ����λ����ǰ�� */
            i--;          
            if (i < 0)
            {
                break;
            }
        }

        /* ������ǰ��û�ҵ�,�������� */
        if (i < 0)
        {
            i = pos;
            while (0 == historyRoads.at(i).x)
            {
                i++;
            }
        }

        noZeroPoint = Point(i, historyRoads.at(i).x);

    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : GetRealRoad
  ��������  : ���Ԥ�⣬���ʵ�ʵ�
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��28��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetRealRoad(Point& detectPoint, Mat& curveCoefficient, Point& realPoint)
{
    ULONG ret = HCOM_OK;
    Point predictPoint;
    INT onCurveError = 2;  
    
    if (NULL == curveCoefficient.data)
    {
        realPoint = detectPoint;
        return ret;
    }

    predictPoint.x = FIT_NUM;

    /* �������߷���Ԥ�� */
    predictPoint.y = (INT)(curveCoefficient.at<DOUBLE>(0, 0) + curveCoefficient.at<DOUBLE>(1, 0) * predictPoint.x +  
        curveCoefficient.at<DOUBLE>(2, 0) * pow(predictPoint.x, 2) + curveCoefficient.at<DOUBLE>(3, 0) * pow(predictPoint.x, 3));  

    if (abs(detectPoint.x - predictPoint.y) < onCurveError)
    {
        realPoint = detectPoint;
    }
    else
    {
        realPoint = Point(predictPoint.y, detectPoint.y);
    }

    
    return ret;
}