// CvPlot - https://github.com/Profactor/cv-plot
// Copyright (c) 2019 by PROFACTOR GmbH - https://www.profactor.at/

#pragma once

#include <CvPlot/libdef.h>
#include <opencv2/core.hpp>
#include <CvPlot/Internal/no_warning.h>
#include <CvPlot/Internal/Pimpl.h>
#include <CvPlot/core/Drawable.h>

namespace CvPlot {

class CVPLOT_LIBRARY_INTERFACE CBox : public Drawable {
public:
    CBox(int colormap, double min, double max);
    CBox(int colormap, double colormap_min, double colormap_max, double colorbox_min, double colorbox_max);
    ~CBox();
    void render(RenderTarget &renderTarget)override;
    bool getBoundingRect(cv::Rect2d &rect)override;
    const std::vector<double> &getTicks()const;
    int getWidth()const;
    CBox& setColormap(int colormap);
    int getColormap()const;
    CBox& setMinMax(double min, double max);
    CBox& setMinMax(double colormap_min, double colormap_max, double colorbox_min, double colorbox_max);
    CBox& setLabel(const std::string &label);
    std::string getLabel();
private:
    class Impl;
    CVPLOT_NO_WARNING_DLL_INTERFACE(Internal::Pimpl<Impl>, impl);
};

}

#ifdef CVPLOT_HEADER_ONLY
#include <CvPlot/imp/CBox.ipp>
#endif
