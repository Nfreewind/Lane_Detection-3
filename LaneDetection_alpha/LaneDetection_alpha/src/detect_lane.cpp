#include "include/def.h"
#include "include//detect_lane.h"

ULONG FilteWidthEachRow(Mat& srcFrame, Mat& dstFrame, vector<Point>& candidatePoints);
ULONG ClusteLanePoints(vector<Point>& candidatePoints, vector<vector<Point> >&clusters);
ULONG FittingClusterLanes(vector<vector<Point> >& clusters, vector<LANE>& clusterFittedLanes);
ULONG FittingStraightLine(vector<Point> point, LANE& fittedLane);
ULONG GetMostSimilarCluster(Point& point, vector<vector<Point> >& clusters, INT& mostSimilar, INT& clusterIndex);
ULONG SelectMultiLaneBaseOnMinWidth(LANE& oneLane, vector<LANE>& multiLanes, vector<LANE>& lanes);
ULONG SelectParallelLanes(vector<LANE>& leftLanes, vector<LANE>& rightLanes, LANE& leftLane, LANE& rightLane);
ULONG SelectBaseOnLength(vector<LANE>& multiLanes);
ULONG SelectCarNearestLane(vector<LANE>& multiLanes, LANE& oneLane,INT& index);
bool comp(LANE& a, LANE& b);
ULONG DeleteAbnormalLanes(vector<LANE>& lanes, vector<LANE>& lastLanes);
ULONG DeleteLanesNearMid(vector<LANE>& multiLanes, vector<LANE>& midLanes);

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
ULONG SelectLane(vector<LANE>& leftLanes, vector<LANE>& rightLanes, vector<LANE>& lastLanes, vector<LANE>& lanes){

    ULONG ret =  HCOM_OK;
    vector<LANE> selectedLeftLanes;
    vector<LANE> selectedRightLanes;
    LANE leftLane;
    LANE rightLane;
    INT index;
    vector<LANE> midLanes;

    /* ���ݳ���ɸѡ */
    if (1 < leftLanes.size())
    {
       ret = SelectBaseOnLength(leftLanes);
    }

    if (1 < rightLanes.size())
    {
        ret = SelectBaseOnLength(rightLanes);
    }

    ret = GetMidLanes(leftLanes, rightLanes, midLanes);
    if (1 < leftLanes.size())
    {
        ret = DeleteLanesNearMid(leftLanes, midLanes);
    }

    if (1 < rightLanes.size())
    {
        ret = DeleteLanesNearMid(rightLanes, midLanes);
    }
    
    /* ���߾�ֻ��һ�����ж������ļ�� */
    if (1 == leftLanes.size() && 1 == rightLanes.size())
    {
        ret = SelectParallelLanes(leftLanes, rightLanes, leftLane, rightLane);
    }

    /* ĳ��û�У� ��һ���� */
    if (0 == leftLanes.size() && 0 != rightLanes.size())
    {
        ret = SelectCarNearestLane(rightLanes, rightLane, index); 
    }
    
    if (0 == rightLanes.size() && 0 != leftLanes.size())
    {
        rightLane = lastLanes[1];
        ret = SelectCarNearestLane(leftLanes, leftLane, index); 
    }
   
    /* ĳ��ֻ��һ������һ�߶��������ڵ�����ɸѡ��һ�� */
    if (1 == leftLanes.size() && 1 != rightLanes.size())
    {
        leftLane = leftLanes[0];

        ret = SelectMultiLaneBaseOnMinWidth(leftLane, rightLanes, selectedRightLanes);

        if (1 <= selectedRightLanes.size())
        {
            ret = SelectCarNearestLane(selectedRightLanes, rightLane, index);          
        }
    }
    
    if (1 == rightLanes.size() && 1 != leftLanes.size())
    {
        rightLane = rightLanes[0];

        ret = SelectMultiLaneBaseOnMinWidth(rightLane, leftLanes, selectedLeftLanes);

        if (1 <= selectedLeftLanes.size())
        {
            ret = SelectCarNearestLane(selectedLeftLanes, leftLane, index);
        }
    }
    
    /* ���߶��Ƕ��� */
    if (1 < leftLanes.size() && 1 < rightLanes.size())
    {
        index = 0;
        while (leftLanes.size() > 1)
        {
            /* ȡ���������� */
            ret = SelectCarNearestLane(leftLanes, leftLane, index);

            ret = SelectMultiLaneBaseOnMinWidth(leftLane, rightLanes, selectedRightLanes);

            if (1 <= selectedRightLanes.size())
            {
                ret = SelectCarNearestLane(selectedRightLanes, rightLane, index);
                break;
            }
            else
            {
                leftLanes.erase(leftLanes.begin() +index);
            }
        }
    }

    /* ������ĳ��δ������ ����һ֡���������*/
    if (0 == leftLane.headPoint.x && 0 == leftLane.backPoint.x)
    {
        leftLane = lastLanes[0];
    }
    if (0 == leftLane.backPoint.x && 0 == leftLane.backPoint.x)
    {
        rightLane = lastLanes[1];
    }

    selectedRightLanes.clear();
    selectedLeftLanes.clear();

    lanes.push_back(leftLane);
    lanes.push_back(rightLane);

    return ret ;
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
ULONG SelectBaseOnLength(vector<LANE>& multiLanes){

    ULONG ret = HCOM_OK;
    INT errorWithHeight = 20;
    INT errorWithSecond = 300;
    LANE longestLane;

    if (1 == multiLanes.size())
    {
        return ret;
    }

    /* ����������,�Ӵ�С */
    sort(multiLanes.begin(),multiLanes.end(),comp);

    /* ����Ľӽ�ͼ��߻�ԶԶ���ڵڶ����ģ���Ϊ��ļ�Ϊ������ */
    if (HEIGHT - multiLanes[0].length < errorWithHeight 
        || multiLanes[0].length - multiLanes[1].length > errorWithSecond)
    {
        longestLane = multiLanes[0];
        multiLanes.clear();
        multiLanes.push_back(longestLane);
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
            && (abs(headDistance) > MIN_DOUBLE_LANE_WIDTH)
            && (abs(backDistance) > MIN_DOUBLE_LANE_WIDTH))
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
ULONG SelectParallelLanes(vector<LANE>& leftLanes, vector<LANE>& rightLanes, LANE& leftLane, LANE& rightLane)
{
    
    ULONG ret =  HCOM_OK;
    INT headDistance = 0;
    INT backDistance = 0;
    INT error = 20;     //������������ƽ�е����

    headDistance = leftLanes[0].headPoint.x - rightLanes[0].headPoint.x;
    backDistance = leftLanes[0].backPoint.x - rightLanes[0].backPoint.x;

    /* ���������ߵ���β��Ƚ��ƣ�ƽ�У����ҿ����һ����Χ�� */
    if ((abs(headDistance - backDistance) < error)
        && (headDistance * backDistance > 0)
        && (abs(headDistance) > MIN_DOUBLE_LANE_WIDTH)
        && (abs(backDistance) > MIN_DOUBLE_LANE_WIDTH))
    {
        leftLane = leftLanes[0];
        rightLane = rightLanes[0];
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
                if (endPos - startPos >= MIN_SINGLE_LANE_WIDTH && endPos - startPos <= MAX_SINGLE_LANE_WIDTH)
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
    DOUBLE sople;
    DOUBLE base;

    if (0 == point.size())
    {
        return ret;
    }

    fitLine(point, linePara, CV_DIST_L2, 0, 0.001, 0.001);    /*��С����*/

    sople = linePara[1] / linePara[0];
    base = linePara[3] - sople * linePara[2];

    fittedLane.sople = sople;
    fittedLane.base = base;
    fittedLane.headPoint = Point(INT((-base) / sople) ,0);
    fittedLane.backPoint = Point(INT((HEIGHT - base) / sople) ,HEIGHT);

    return ret;
}

/*****************************************************************************
  �� �� ��  : SelectLaneBaseOnMultiFrames
  ��������  : ��������֡ɸѡ������
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
ULONG SelectLaneBaseOnMultiFrames(vector<LANE>& leftLanes, vector<LANE>& rightLanes, vector<LANE>& lastLeftlanes, vector<LANE>& lastRightLanes)
{
    ULONG ret = HCOM_OK;

    


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
ULONG GetMidLanes(vector<LANE>& leftLanes, vector<LANE>& rightLanes, vector<LANE>& midLanes)
{
    ULONG ret = HCOM_OK;
    LANE midLane;

    for (size_t i = 0; i < leftLanes.size(); i++)
    {
        for (size_t j = 0; j < rightLanes.size(); j++)
        {
            midLane.headPoint = Point((leftLanes[i].headPoint.x + rightLanes[j].headPoint.x) / 2 , 0);
            midLane.backPoint = Point((leftLanes[i].backPoint.x + rightLanes[j].backPoint.x) / 2 , HEIGHT);
            midLanes.push_back(midLane);
        }

    }

    return ret;
}