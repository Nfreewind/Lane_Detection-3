#include "include/def.h"
#include "include/preprocess.h"

/*****************************************************************************
  �� �� ��  : resizeSrcFrame
  ��������  : �ü�����ͼ�� ������Ҹ���Ȥ����
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
ULONG GetRoiFrame(Mat& srcFrame, Mat& roundView, Mat& leftRoiFrame, Mat& rightRoiFrame){

    ULONG ret = HCOM_OK;

    if (NULL_PTR == srcFrame.data)
    {
        return HCOM_ERR;
    }

    Rect roundViewRoi(0, 0, WIDTH, HEIGHT);
    Rect carRegion(140, 150, 120, 300);
    Mat dstFrame;
    Mat carRegionFrame;

    dstFrame = srcFrame(roundViewRoi);
    dstFrame.copyTo(roundView);
    
    /* ���ú�ɫ */
    carRegionFrame = srcFrame(carRegion);
    carRegionFrame.setTo(cv::Scalar(0, 0, 0));

    leftRoiFrame = dstFrame(Rect(0, 0, ROI_WIDTH, HEIGHT));
    rightRoiFrame = dstFrame(Rect(RIGHT_ROI_POS, 0, ROI_WIDTH, HEIGHT));

    return ret;
}

/*****************************************************************************
  �� �� ��  : ConvertToBind
  ��������  : ������ɫ��������ȡ��ֵͼ��
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
ULONG ConvertToBind(Mat& srcFrame, Mat& dstFrame){

    ULONG ret = HCOM_OK;
    Mat hlsFrame;
    vector<Mat> hslChannels;
    vector<Mat> rgbChannels;
    Mat sChannel;
    Mat rChannel;
    Mat rsAddFrame;
    INT bindThreshold = 90;

    /* ת��HLS�ռ� */
    cvtColor(srcFrame, hlsFrame,COLOR_BGR2HLS);

    /* �����Sͨ�� */
    split(hlsFrame, hslChannels);
    sChannel = hslChannels.at(2);
    //imshow("s", sChannel);

    /* ����Rͨ�� */
    split(srcFrame, rgbChannels);
    rChannel = rgbChannels.at(2);
    //imshow("r", rChannel);

    /* R��Sͨ����Ȩ�� */
    addWeighted(sChannel, 0.5, rChannel, 0.5, 0, rsAddFrame);
    //imshow("r+s", rsAddFrame);

    /* ��ֵ�� */
    threshold(rsAddFrame, dstFrame, bindThreshold, 255, CV_THRESH_BINARY);

    return ret;

}