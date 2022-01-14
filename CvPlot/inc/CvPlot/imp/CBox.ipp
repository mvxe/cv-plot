// CvPlot - https://github.com/Profactor/cv-plot
// Copyright (c) 2019 by PROFACTOR GmbH - https://www.profactor.at/

#pragma once

#include <CvPlot/drawables/CBox.h>
#include <CvPlot/Internal/util.h>
#include <opencv2/opencv.hpp>

namespace CvPlot {

class CBox::Impl {
public:
    const int _fontFace = cv::FONT_HERSHEY_SIMPLEX;
    const double _fontScale = .4;
    const int _fontThickness = 1;
    bool _locateRight = false;
    cv::Scalar _color = cv::Scalar(0, 0, 0);
    std::vector<double> _ticks;
    bool _isLogarithmic;
    int _width = 0;
    int _colormap;
    double _colormap_min;
    double _colormap_max;
    double _colorbox_min;
    double _colorbox_max;
    std::string _label;

    void calcTicks(const RenderTarget & renderTarget) {
        cv::Rect innerRect = renderTarget.innerRect();
        double y0 = _colorbox_min;
        double y05 = (_colorbox_max+_colorbox_min)/2;
        double y1 = _colorbox_max;
        if (y1 == y0 || !std::isfinite(y0) || !std::isfinite(y05) || !std::isfinite(y1)) {
            _ticks = { y0 };
            return;
        }
        if (y1 < y0) {
            std::swap(y0, y1);
        }
        int labelHeight = getTextSize("1,2").height;
        int spacing = 20;
        int estimatedTickCount = (int)std::ceil(innerRect.height / (labelHeight + spacing));
        double epsilon = 1e-5;
        _isLogarithmic = std::abs(2 * (y05 - y0) / (y1 - y0) - 1) > epsilon;
        if (_isLogarithmic) {
            _ticks = Internal::calcTicksLog(y0, y1, estimatedTickCount);
        } else {
            _ticks = Internal::calcTicksLinear(y0, y1, estimatedTickCount);
        }
    }
    cv::Size getTextSize(const std::string &text) {
        int baseline;
        return cv::getTextSize(text, _fontFace, _fontScale, _fontThickness, &baseline);
    }
    void render(RenderTarget & renderTarget) {
        cv::Mat3b &outerMat = renderTarget.outerMat();
        const cv::Rect &innerRect = renderTarget.innerRect();
        if (!innerRect.area()) {
            return;
        }
        calcTicks(renderTarget);
        _width = 0;
        const int box_width = 20;
        const int box_margin = 10;
        for (double tick : _ticks) {
            int tickPix = (int)((1-(tick-_colorbox_min)/(_colorbox_max-_colorbox_min))*innerRect.height + .5);
            std::string label = Internal::format(tick, _isLogarithmic);
            cv::Size size = getTextSize(label);
            cv::Point labelPos;
            cv::Point tickPos;
            const int margin = 10;
            const int tickLength = 4;
            labelPos.x = innerRect.x + innerRect.width + margin + box_width + box_margin;
            tickPos.x = innerRect.x + innerRect.width + box_margin;

            labelPos.y = (int)(innerRect.y + tickPix + size.height / 2);
            tickPos.y = innerRect.y + tickPix;

            cv::putText(outerMat, label, labelPos, _fontFace, _fontScale, _color, _fontThickness, cv::LINE_AA);
            cv::line(outerMat, tickPos, tickPos + cv::Point(tickLength-1,0), _color);
            cv::line(outerMat, tickPos + cv::Point(box_width-tickLength-1,0), tickPos + cv::Point(box_width-2,0), _color);
            if (margin + box_width + box_margin + size.width / 2 > _width) {
                _width = margin + box_width + box_margin + size.width / 2;
            }
        }
        int baseline;
        cv::Size textSize = cv::getTextSize(_label, _fontFace, _fontScale, _fontThickness, &baseline);
        int margin = _fontThickness + 5; //required for cv::LINE_AA
        cv::Mat3b temp(textSize.height + 2 * margin, textSize.width + baseline + 2 * margin, cv::Vec3b::all(255));
        cv::Point pos((temp.cols - textSize.width) / 2, textSize.height + margin);
        cv::putText(temp, _label, pos, _fontFace, _fontScale, _color, _fontThickness, cv::LINE_AA);
        temp = temp.t();
        cv::flip(temp, temp, 0);
        cv::Point labelTopLeft(innerRect.x + innerRect.width + _width + 5 + temp.cols, innerRect.y + innerRect.height / 2 - temp.rows / 2);
        Internal::paint(temp, outerMat, labelTopLeft);
        renderBox(renderTarget, box_width, box_margin);
    }
    void renderBox(RenderTarget & renderTarget, const int width, const int margin){
        cv::Mat3b &outerMat = renderTarget.outerMat();
        const cv::Rect &innerRect = renderTarget.innerRect();

        cv::Mat amat(innerRect.height+1, 1, CV_8U);
        double min=(_colorbox_min-_colormap_min)/(_colormap_max-_colormap_min)*255;
        double max=(_colorbox_max-_colormap_min)/(_colormap_max-_colormap_min)*255;
        for(int i=0; i!=innerRect.height+1; i++) amat.at<uchar>(innerRect.height-i)=min+i*(max-min)/(innerRect.height);
        if(_colormap>0) {
            cv::applyColorMap(amat, amat, _colormap);
            cv::cvtColor(amat, amat, cv::COLOR_BGR2RGB);
        }else {
            cv::cvtColor(amat, amat, cv::COLOR_GRAY2RGB);
        }

        int top = innerRect.y - 1;
        int bottom = innerRect.y + innerRect.height;
        int x = innerRect.x + innerRect.width + margin - 1;
        int cwidth = std::min(width, outerMat.cols-x);
        int cheight = std::min(innerRect.height+1, outerMat.rows-top);
        if(cwidth>0 && cheight>0)
            cv::repeat(amat, 1, cwidth, cv::Mat(outerMat,{x,top,cwidth,cheight}));
        cv::rectangle(outerMat, cv::Point(x, top), cv::Point(x+width, bottom), _color);
    }
};

CVPLOT_DEFINE_FUN
CBox::~CBox() {
}

CVPLOT_DEFINE_FUN
CBox::CBox(const int colormap, const double min, const double max){
    setColormap(colormap);
    setMinMax(min, max);
}

CVPLOT_DEFINE_FUN
CBox::CBox(const int colormap, const double colormap_min, const double colormap_max, const double colorbox_min, const double colorbox_max){
    setColormap(colormap);
    setMinMax(colormap_min, colormap_max, colorbox_min, colorbox_max);
}

CVPLOT_DEFINE_FUN
CBox& CBox::setColormap(int colormap){
    impl->_colormap = colormap;
    return *this;
}

CVPLOT_DEFINE_FUN
int CBox::getColormap()const{
    return impl->_colormap;
}

CVPLOT_DEFINE_FUN
CBox& CBox::setMinMax(double min, double max){
    setMinMax(min, max, min, max);
}

CVPLOT_DEFINE_FUN
CBox& CBox::setMinMax(double colormap_min, double colormap_max, double colorbox_min, double colorbox_max){
    impl->_colormap_min=colormap_min;
    impl->_colormap_max=colormap_max;
    impl->_colorbox_min=colorbox_min;
    impl->_colorbox_max=colorbox_max;
    if(impl->_colorbox_min<impl->_colormap_min) impl->_colorbox_min=impl->_colormap_min;
    if(impl->_colorbox_max>impl->_colormap_max) impl->_colorbox_max=impl->_colormap_max;
}

CVPLOT_DEFINE_FUN
void CBox::render(RenderTarget & renderTarget){
    impl->render(renderTarget);
}

CVPLOT_DEFINE_FUN
const std::vector<double> & CBox::getTicks() const{
    return impl->_ticks;
}

CVPLOT_DEFINE_FUN
int CBox::getWidth()const {
    return impl->_width;
}

CVPLOT_DEFINE_FUN
bool CBox::getBoundingRect(cv::Rect2d &rect) {
    return false;
}

CVPLOT_DEFINE_FUN
CBox & CBox::setLabel(const std::string & label){
    impl->_label = label;
    return *this;
}

CVPLOT_DEFINE_FUN
std::string CBox::getLabel(){
    return impl->_label;
}

}
