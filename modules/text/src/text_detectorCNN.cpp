// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#include "precomp.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core.hpp"
#include "opencv2/dnn.hpp"

#include <fstream>
#include <algorithm>

using namespace cv::dnn;

namespace cv
{
namespace text
{

class TextDetectorCNNImpl : public TextDetectorCNN
{
protected:
    Net net_;
    std::vector<Size> sizes_;
    int inputChannelCount_;
    bool detectMultiscale_;


    void getOutputs(const float* buffer,int nbrTextBoxes,int nCol,
                               std::vector<Rect>& Bbox, std::vector<float>& confidence, Size inputShape)
    {
        for(int k = 0; k < nbrTextBoxes; k++)
        {
            float x_min = buffer[k*nCol + 3]*inputShape.width;
            float y_min = buffer[k*nCol + 4]*inputShape.height;

            float x_max = buffer[k*nCol + 5]*inputShape.width;
            float y_max = buffer[k*nCol + 6]*inputShape.height;

            CV_Assert(x_min < x_max, y_min < y_max);

            x_min = std::max(0.f, x_min);
            y_min = std::max(0.f, y_min);

            x_max = std::min(inputShape.width - 1.f,  x_max);
            y_max = std::min(inputShape.height - 1.f,  y_max);

            int wd = cvRound(x_max - x_min);
            int ht = cvRound(y_max - y_min);

            Bbox.push_back(Rect(cvRound(x_min), cvRound(y_min), wd, ht));
            confidence.push_back(buffer[k*nCol + 2]);
        }
    }

public:
    TextDetectorCNNImpl(const String& modelArchFilename, const String& modelWeightsFilename, bool detectMultiscale) :
        detectMultiscale_(detectMultiscale)
    {
        net_ = readNetFromCaffe(modelArchFilename, modelWeightsFilename);
        CV_Assert(!net_.empty());
        inputChannelCount_ = 3;
        sizes_.push_back(Size(700, 700));

        if(detectMultiscale_)
        {
            sizes_.push_back(Size(300, 300));
            sizes_.push_back(Size(700,500));
            sizes_.push_back(Size(700,300));
            sizes_.push_back(Size(1600,1600));
        }
    }

    void detect(InputArray inputImage_, std::vector<Rect>& Bbox, std::vector<float>& confidence)
    {
        CV_Assert(inputImage_.channels() == inputChannelCount_);
        Size inputSize = inputImage_.getMat().size();
        Bbox.resize(0);
        confidence.resize(0);

        for(size_t i = 0; i < sizes_.size(); i++)
        {
            Size inputGeometry = sizes_[i];
            Mat inputImage = inputImage_.getMat().clone();
            resize(inputImage, inputImage, inputGeometry);
            net_.setInput(blobFromImage(inputImage, 1, inputGeometry, Scalar(123, 117, 104)), "data");
            Mat outputNet = net_.forward();
            int nbrTextBoxes = outputNet.size[2];
            int nCol = outputNet.size[3];
            int outputChannelCount = outputNet.size[1];
            CV_Assert(outputChannelCount == 1);
            getOutputs((float*)(outputNet.data), nbrTextBoxes, nCol, Bbox, confidence, inputSize);
        }
     }
};

Ptr<TextDetectorCNN> TextDetectorCNN::create(const String &modelArchFilename, const String &modelWeightsFilename, bool detectMultiscale)
{
    return makePtr<TextDetectorCNNImpl>(modelArchFilename, modelWeightsFilename, detectMultiscale);
}
} //namespace text
} //namespace cv
