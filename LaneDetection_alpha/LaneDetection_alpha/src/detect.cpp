#include "include/def.h"
#include "include//detect.h"

ULONG FilteWidthEachRow(Mat& srcFrame, Mat& dstFrame, vector<Point>& candidatePoints);
ULONG ClusteLanePoints(vector<Point>& candidatePoints, vector<vector<Point> >&clusters);
ULONG FittingClusterLanes(vector<vector<Point> >& clusters, vector<LANE>& clusterFittedLanes);
ULONG FittingStraightLine(vector<Point> point, LANE& fittedLane);
ULONG GetMostSimilarCluster(Point& point, vector<vector<Point> >& clusters, INT& mostSimilar, INT& clusterIndex);
ULONG SelectMultiLaneBaseOnMinWidth(LANE& oneLane, vector<LANE>& multiLanes, vector<LANE>& lanes);
ULONG SelectLongestLane(vector<LANE>& lanes, LANE& longestLane);
ULONG SelectCarNearestLane(vector<LANE>& multiLanes, LANE& oneLane,INT& index);
bool comp(LANE& a, LANE& b);
ULONG DeleteAbnormalLanes(vector<LANE>& lanes, vector<LANE>& lastLanes);
ULONG DeleteLanesNearMid(vector<LANE>& multiLanes, vector<LANE>& midLanes);
ULONG DeleteOutlierLane(ROAD& road, vector<ROAD> lastRoads);
ULONG CalRoadWidth(ROAD& road);
ULONG GenUnkwonLaneOnWidth(LANE& knowLane, LANE& unknowLane, DOUBLE& roadWidth);
ULONG DeleteLaneCoveredByCar(ROAD& road);
ULONG SelectParallelLanes(ROAD& road);
ULONG CalOneStepDiffer(ROAD& presentRoad, vector<ROAD>& lastRoads, vector<INT>& differs);

/*****************************************************************************
  �� �� ��  : DetectLane
  ��������  : �ڶ�ֵ����ͼ��Ļ����Ͻ��г����ߵļ��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG DetectLane(Mat& srcFrame, Mat& bindFrame, vector<vector<Point> >& clusters, vector<LANE>& clusterFittedLanes){

    ULONG ret = HCOM_OK;

    vector<Point> candidatePoints; 

    /* ȥ�� */
    ret = FilteWidthEachRow(srcFrame, bindFrame, candidatePoints);

    /* �ִ� */
    ret = ClusteLanePoints(candidatePoints, clusters);     

    /* ��� */
    ret = FittingClusterLanes(clusters, clusterFittedLanes);

    return ret;
}

/*****************************************************************************
  �� �� ��  : SelectLanes
  ��������  : �Լ����ĳ����߽��н�һ����ɸѡ
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetDetectedRoad(vector<LANE>& leftLanes, vector<LANE>& rightLanes, vector<ROAD>& lastRoads, DOUBLE& roadWidth, ROAD& detectRoad)
{

    ULONG ret =  HCOM_OK;

    /* ѡ�� */
    if (0 != leftLanes.size())
    {
        ret = SelectLongestLane(leftLanes, detectRoad.leftLane); 
    }
    if (0 != rightLanes.size())
    {
        ret = SelectLongestLane(rightLanes, detectRoad.rightLane);
    }

    /* �������ߣ������·��� */
    if (0 != detectRoad.leftLane.length && 0 != detectRoad.rightLane.length)
    {
        ret = CalRoadWidth(detectRoad);

        if (detectRoad.width < MIN_ROAD_WIDTH)
        {
            /* ���С�ڳ���ɾ���������ڷ�Χ���ǵ��� */
            ret = DeleteLaneCoveredByCar(detectRoad);
        }
    }

    /* ������һ�� */
    if (0 != detectRoad.leftLane.length && 0 == detectRoad.rightLane.length)
    {
        ret = GenUnkwonLaneOnWidth(detectRoad.leftLane, detectRoad.rightLane, roadWidth);
        detectRoad.width = roadWidth;
    }
    if (0 != detectRoad.rightLane.length && 0 == detectRoad.leftLane.length)
    {     
        ret = GenUnkwonLaneOnWidth(detectRoad.rightLane, detectRoad.leftLane, roadWidth);
        detectRoad.width = roadWidth;   
    }


    /* ת��Ϊ������ */
    ret = CvtLaneToPolarCoodinate(detectRoad.leftLane);

    ret = CvtLaneToPolarCoodinate(detectRoad.rightLane);

    /* ɾ����Ⱥ�� */
    ret = DeleteOutlierLane(detectRoad, lastRoads);

    return ret ;
}

ULONG CvtLaneToRectCoordinate(LANE& lane)
{
    ULONG ret = HCOM_OK;

    lane.headPoint.x = lane.s + WIDTH / 2;
    lane.headPoint.y = 0;

    lane.backPoint.y = HEIGHT;
    lane.backPoint.x =  (lane.backPoint.y - lane.headPoint.y) / tan(lane.theta) + lane.headPoint.x;

    lane.length = HEIGHT;

    return ret;
}

ULONG CvtLaneToPolarCoodinate(LANE& lane)
{
    ULONG ret = HCOM_OK;
    FLOAT sople = 0;

    if (0 == lane.length)
    {
        return ret;
    }

    lane.s = lane.headPoint.x - WIDTH / 2;

    if (lane.headPoint.x == lane.backPoint.x)
    {
        lane.theta = PI / 2;
    }
    else
    {
        lane.theta = atan2f(lane.backPoint.y - lane.headPoint.y, lane.backPoint.x - lane.headPoint.x);
    }


    return ret;
}

/*****************************************************************************
  �� �� ��  : GetMidLanes
  ��������  : ������
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��14��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetMidLanes(ROAD& road)
{
    ULONG ret = HCOM_OK;

    road.middleLane.headPoint = Point((road.leftLane.headPoint.x + road.rightLane.headPoint.x) / 2 , 0);
    road.middleLane.backPoint = Point((road.leftLane.backPoint.x + road.rightLane.backPoint.x) / 2 , HEIGHT);
    road.middleLane.length = HEIGHT;

    return ret;
}

/*****************************************************************************
  �� �� ��  : DeleteLaneCoveredByCar
  ��������  : ��ȹ�Сʱ��ɾ�����������򸲸ǵ��ߣ�Ŀ�����˳���ͷ�ĸ���
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
ULONG DeleteLaneCoveredByCar(ROAD& road)
{
    ULONG ret =  HCOM_OK;
    INT midPos;

    midPos = (road.leftLane.headPoint.x + road.leftLane.backPoint.x) / 2; 
    if (midPos > CAR_POS && midPos < (CAR_POS + MIN_ROAD_WIDTH))
    {
        road.leftLane.backPoint = Point(0,0);
        road.leftLane.headPoint = Point(0,0);
        road.leftLane.length = 0;
        road.width = 0;
    }

    midPos = (road.rightLane.headPoint.x + road.rightLane.backPoint.x) / 2; 
    if (midPos > CAR_POS && midPos < (CAR_POS + MIN_ROAD_WIDTH))
    {
        road.rightLane.backPoint = Point(0,0);
        road.rightLane.headPoint = Point(0,0);
        road.rightLane.length = 0;
        road.width = 0;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : DeleteArrowLane
  ��������  : ɾ����Ⱥ��
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
ULONG DeleteOutlierLane(ROAD& road, vector<ROAD> lastRoads)
{
    ULONG ret =  HCOM_OK;

    const INT outTrh = 50;

    LANE resetLane = {Point(0,0), Point(0,0), 0};
    INT lastMiddlePoint = 0;

    if (0 == lastRoads.size())
    {
        return ret;
    }

    lastMiddlePoint = (lastRoads.back().leftLane.headPoint.x + lastRoads.back().rightLane.headPoint.x) / 2;

    /* ������һ֡�������е��غϣ���Ϊ����� */
    if (abs(road.leftLane.headPoint.x - lastMiddlePoint) < outTrh
        || abs(road.rightLane.headPoint.x - lastMiddlePoint) < outTrh)
    {
        road.leftLane = resetLane;
        road.rightLane = resetLane;
        road.width = 0;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : GenUnkwonLaneOnWidth
  ��������  : �����Ѽ��������ƶϳ���һ����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GenUnkwonLaneOnWidth(LANE& knowLane, LANE& unknowLane, DOUBLE& roadWidth)
{
    ULONG ret =  HCOM_OK;
    INT flag = 1;

    if (0 == roadWidth)
    {
        return HCOM_ERR;
    }

    flag = (knowLane.headPoint.x < ROI_WIDTH) ? 1 : -1;

    unknowLane.headPoint.x = knowLane.headPoint.x + roadWidth * flag;
    unknowLane.headPoint.y = 0;
    unknowLane.backPoint.x = knowLane.backPoint.x + roadWidth * flag;
    unknowLane.backPoint.y = HEIGHT;
    unknowLane.length = knowLane.length;

    return ret ;
}

/*****************************************************************************
  �� �� ��  : CalDoubleLanesWidth
  ��������  : ����������֮��Ŀ��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG CalRoadWidth(ROAD& road)
{
    ULONG ret = HCOM_OK;
    DOUBLE width;

    /* ���߽���ƽ�У�ȡ��β���ƽ��ֵ */
    width = (road.rightLane.backPoint.x - road.leftLane.backPoint.x) 
        + (road.rightLane.headPoint.x - road.leftLane.headPoint.x);
    road.width = width /2.0;
    
    return ret; 

}


/*****************************************************************************
  �� �� ��  : ResizeRightLanes
  ��������  : ���Ҳ���߷��ص�ԭͼ����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG ResizeRightLanes(vector<LANE>& rightLanes, INT offset)
{
    ULONG ret = HCOM_OK;

    /* ���Ҳ������ع���ԭͼ */
    for (size_t i = 0; i < rightLanes.size(); i++)
    {      
        rightLanes[i].headPoint.x = rightLanes[i].headPoint.x + offset;
        rightLanes[i].backPoint.x = rightLanes[i].backPoint.x + offset;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : comp
  ��������  : ��������
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
bool comp(LANE& a, LANE& b)
{
    return a.length > b.length;
}

/*****************************************************************************
  �� �� ��  : SelectLongestLane
  ��������  : ����ĳ�����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG SelectLongestLane(vector<LANE>& lanes, LANE& longestLane){

    ULONG ret = HCOM_OK;
    INT maxLength = 0;

    /* ��Ϊ��ļ�Ϊ������ */
    for (size_t i = 0; i < lanes.size(); i++)
    {
        if (lanes[i].length > maxLength && lanes[i].length > MIN_LANE_LENGTH)
        {
            longestLane = lanes[i];
            maxLength = lanes[i].length;
        }
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : SelectLongestLane
  ��������  : ��������ĳ�����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG SelectCarNearestLane(vector<LANE>& multiLanes, LANE& oneLane, INT& index){
    
    ULONG ret = HCOM_OK;
    INT minDis;

    if (1 == multiLanes.size())
    {
        return ret;
    }

    minDis = abs(WIDTH / 2 - multiLanes[0].headPoint.x);
    for (size_t i = 0; i < multiLanes.size(); i++)
    {
        if (minDis >= abs(WIDTH / 2 - multiLanes[i].headPoint.x))
        {
            minDis = abs(WIDTH / 2 - multiLanes[i].headPoint.x);
            oneLane = multiLanes[i];
            index = i;
        }
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : DeleteLanesNearMid
  ��������  : 
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG DeleteLanesNearMid(vector<LANE>& multiLanes, vector<LANE>& midLanes)
{
    ULONG ret = HCOM_OK;
    INT error = 150;    

    BOOL isError = FALSE;
    vector<LANE> tmpLanes;

    for (size_t i = 0 ; i < multiLanes.size(); i ++)
    {
        for (size_t j = 0; j < midLanes.size(); j++)
        {
            if (abs(multiLanes[i].headPoint.x - midLanes[j].headPoint.x) < error)
            {
                isError = TRUE;
            }
        }

        if (!isError)
        {
            tmpLanes.push_back(multiLanes[i]);
        }
        isError = FALSE;
    }
    multiLanes.clear();
    multiLanes = tmpLanes;
    tmpLanes.clear();

    return ret;

}

/*****************************************************************************
  �� �� ��  : SelectBaseOnMinWidth
  ��������  : ������С���ɸѡ
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG SelectMultiLaneBaseOnMinWidth(LANE& oneLane, vector<LANE>& multiLanes, vector<LANE>& lanes){
    
    ULONG ret =  HCOM_OK;
    LANE tmpLane;
    INT headDistance = 0;
    INT backDistance = 0;
    INT error = 20;     //������������ƽ�е����

    for (size_t i = 0 ; i < multiLanes.size(); i++)
    {
        tmpLane = multiLanes[i];
        headDistance = tmpLane.headPoint.x - oneLane.headPoint.x;
        backDistance = tmpLane.backPoint.x - oneLane.backPoint.x;

        /* ���������ߵ���β��Ƚ��ƣ�ƽ�У����ҿ����һ����Χ�� */
        if ((abs(headDistance - backDistance) < error)
            && (headDistance * backDistance > 0)   
            && (abs(headDistance) > MIN_ROAD_WIDTH)
            && (abs(backDistance) > MIN_ROAD_WIDTH))
        {
            lanes.push_back(tmpLane);
        }
    }

    return ret ;
}

/*****************************************************************************
  �� �� ��  : SelectSignalLaneBaseOnMinWidth
  ��������  : ������С���ɸѡ
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG SelectParallelLanes(ROAD& road)
{
    
    ULONG ret =  HCOM_OK;
    INT headDistance = 0;
    INT backDistance = 0;
    INT error = 15;     //������������ƽ�е����

    headDistance = road.leftLane.headPoint.x - road.rightLane.headPoint.x;
    backDistance = road.leftLane.backPoint.x - road.rightLane.backPoint.x;

    /* �ж��Ƿ�ƽ�� */
    if ((abs(headDistance - backDistance) > error)
        || headDistance > 0
        || backDistance > 0)
    {
        road.leftLane.length = 0;
        road.rightLane.length = 0;
    }

    return ret ;
}

/*****************************************************************************
  �� �� ��  : FilteWidthEachRow
  ��������  : ���ݳ����߾���һ����ȵ�����ȥ��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG FilteWidthEachRow(Mat& srcFrame, Mat& dstFrame, vector<Point>& candidatePoints){

    ULONG ret = HCOM_OK;

    Mat widthSelectFrame = Mat::zeros(srcFrame.rows, srcFrame.cols, CV_8UC1);
    dstFrame = Mat::zeros(srcFrame.rows, srcFrame.cols, CV_8UC1);

    UCHAR* srcData = srcFrame.data;
    UCHAR* widthSelectData = widthSelectFrame.data;
    UCHAR* dstData = dstFrame.data;

    INT col = 0;  //��
    INT row = 0;  //��
    INT startPos = 0;
    INT endPos = 0;

    /* һ�������Ӧ������Ϊ0 */
    for (row = 0; row < srcFrame.rows; row++)
    {
        while (col < srcFrame.cols - 1)
        {
            if (255 == srcData[row * srcFrame.cols + col])
            {
                startPos = col;   /* ��¼��Ϊ0���صĿ�ʼλ�� */

                /* ���Ҹ���������Ϊ0 ������ */
                while (col < srcFrame.cols - 1)
                {
                    col++;
                    if (0 == srcData[row * srcFrame.cols + col])
                    {
                        break;
                    }
                }
                endPos = col;     /* ��¼��Ϊ0���صĽ���λ�� */

                /* ��ֹ������֮������������صĿ�ȣ�Ϊ��ѡ���������� */
                if (endPos - startPos >= MIN_LANE_WIDTH && endPos - startPos <= MAX_LANE_WIDTH)
                {
                    dstData[startPos + row * srcFrame.cols] = 255;
                    dstData[endPos + row * srcFrame.cols] = 255;
                    candidatePoints.push_back(Point((startPos + endPos) / 2,row));
                }
                col = endPos;
            }
            else
            {
                col++;
            }
        }
        col = 0;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : ClusterSameColumPoints
  ��������  : ���ݺ��������ƶȷִ�
  �������  : ��ѡ�������ϵĵ�
  �������  : ����õ��Ĵ�
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG ClusteLanePoints(vector<Point>& candidatePoints, vector<vector<Point> >& clusters)
{
    ULONG ret = HCOM_OK;

    vector<Point> tmpCluster;
    INT mostSimilarClusterIndex;
    INT mostSimilar;
    INT minError = 8;

    if (0 == candidatePoints.size())
    {
        return ret;
    }

    /* ȡ��һ����ѡ����Ϊ��һ���� */
    tmpCluster.push_back(candidatePoints[0]);
    clusters.push_back(tmpCluster);
    tmpCluster.clear();

    /* ���ݺ�����������ƶȣ��ִ� */
    for (size_t i = 1; i < candidatePoints.size(); i++)
    {
        ret = GetMostSimilarCluster(candidatePoints[i], clusters, mostSimilar, mostSimilarClusterIndex);

        /* �������ƴص����ƶ�С����ֵ����أ�����Ϊ�´� */
        if (mostSimilar < minError)
        { 
            clusters[mostSimilarClusterIndex].push_back(candidatePoints[i]);
        }
        else
        {
            tmpCluster.push_back(candidatePoints[i]);
            clusters.push_back(tmpCluster);
            tmpCluster.clear();
        }
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : GetMostSimilarCluster
  ��������  : �ҳ������ƵĴ�
  �������  : ��ѡ�������ϵĵ�
  �������  : �ص�����
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG GetMostSimilarCluster(Point& point, vector<vector<Point> >& clusters, INT& mostSimilar, INT& clusterIndex){
    
    ULONG ret = HCOM_OK;
    Point clusterLastPoint;
    vector<INT> similar;

    for (size_t i = 0; i < clusters.size(); i++)
    {
        clusterLastPoint = clusters[i].back();
        similar.push_back(abs(point.x - clusterLastPoint.x));
    }

    /* similarԽС��Խ���� */
    mostSimilar = similar[0];
    clusterIndex = 0;
    for (size_t i = 0; i < similar.size(); i++)
    {
        if (similar[i] < mostSimilar)
        {
            mostSimilar = similar[i];
            clusterIndex = i;
        }
    }

    similar.clear();
    return ret;
}

/*****************************************************************************
  �� �� ��  : FitClusterLine
  ��������  : �Ը����ֺõĴؽ������
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG FittingClusterLanes(vector<vector<Point> >& clusters, vector<LANE>& clusterFittedLanes){

    ULONG ret =HCOM_OK;

    vector<Point> cluster;
    LANE clusterFittedLane;

    if (0 == clusters.size())
    {
        return ret;
    }

    for (size_t i = 0; i < clusters.size(); i++)
    {     
        cluster = clusters[i];

        /* �صĴ�С��Ϊ�ߵĳ��� */
        if (cluster.size() > MIN_LANE_LENGTH)
        {
            ret = FittingStraightLine(cluster, clusterFittedLane);
            clusterFittedLane.length = cluster.size();
            clusterFittedLanes.push_back(clusterFittedLane);
        }
        cluster.clear();
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : FittingStraightLine
  ��������  : ����OpenCV��������ֱ�����
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��6��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG FittingStraightLine(vector<Point> point, LANE& fittedLane)
{
    ULONG ret = HCOM_OK;
    Vec4f linePara;
    DOUBLE a;
    DOUBLE b;
    DOUBLE c;

    if (0 == point.size())
    {
        return ret;
    }

    fitLine(point, linePara, CV_DIST_L2, 0, 0.001, 0.001);    /*��С����*/

    /* ��ϳ���ֱ�߷��̼�Ϊ a * x + b * y + c = 0 */
    a = (-1) *linePara[1];
    b = linePara[0];
    c = (-1) * (a * linePara[2] + b * linePara[3]);

    if (0 == a && 0 != b)
    {
        /* ��ʱ���� b * y + c = 0 */
        fittedLane.headPoint = Point(0, (-1) * c / b);
        fittedLane.backPoint = Point(WIDTH, (-1) * c / b);
    }
    if (0 == b && 0 != a)
    {
        /* ��ʱ���� a * x + c = 0 */
        fittedLane.headPoint = Point((-1) * c / a, 0);
        fittedLane.backPoint = Point((-1) * c / a, HEIGHT);
    }

    if (0 != a && 0 != b)
    {
        fittedLane.headPoint = Point( (-1) * (c + b * 0) / a, 0);
        fittedLane.backPoint = Point((-1) * (c + b * HEIGHT) / a, HEIGHT);
    }

    if (0 == a && 0 == b)
    {
        return HCOM_ERR;
    }

    return ret;
}

/*****************************************************************************
  �� �� ��  : DeleteAbnormalLanes
  ��������  : ����һ֡�Ƚϣ��˳���ͻȻ������쳣��
  �������  :  
  �������  :  
  �� �� ֵ  : 
  ���ú���  : N/A
  ��������  : 
  
  �޸���ʷ      :
   1.��    ��   : 2018��3��14��
     ��    ��   : ccy0739
     �޸�����   : �����ɺ���
 *****************************************************************************/
ULONG DeleteAbnormalLanes(vector<LANE>& lanes, vector<LANE>& lastLanes)
{
    ULONG ret = HCOM_OK;
    INT errorPoint = 20;    
    BOOL isError = FALSE;

    vector<LANE> tmpLanes;

    for (size_t i = 0 ; i < lanes.size(); i ++)
    {
        for (size_t j = 0; j < lastLanes.size(); j++)
        {
            /* ������֡�����ĳ����߶���֮�����errorPoint������Ϊ���쳣 */
            if (abs(lanes[i].headPoint.x - lastLanes[j].headPoint.x) > errorPoint)
            {
                isError = TRUE;
            }
        }

        if (!isError)
        {
            tmpLanes.push_back(lanes[i]);
        }
        isError = FALSE;
    }

    /* ��������߶����쳣ͻ��㣬��һ֡�����Ϊ��֡��� */
    if (0 == tmpLanes.size())
    {
        lanes = lastLanes;
    }
    else
    {
        lanes.clear();
        lanes = tmpLanes;
        tmpLanes.clear();
    }

    return ret;
}
