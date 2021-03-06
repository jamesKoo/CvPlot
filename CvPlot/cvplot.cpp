// Matlab style plot functions for OpenCV by Changbo (zoccob@gmail).
#include "cvplot.h"

namespace CvPlot
{

//  use anonymous namespace to hide global variables.
namespace
{
	const Scalar CV_BLACK = CV_RGB(0,0,0);
	const Scalar CV_WHITE = CV_RGB(255,255,255);
	const Scalar CV_GREY = CV_RGB(150,150,150);

	PlotManager pm;
}


Series::Series(void)
{
	data = NULL;
	label = "";
	Clear();
}

Series::Series(const Series& s):count(s.count), label(s.label), auto_color(s.auto_color), color(s.color)
{
	data = new float[count];
	memcpy(data, s.data, count * sizeof(float));
}


Series::~Series(void)
{
	Clear();
}

void Series::Clear(void)
{
	if (data != NULL)
		delete [] data;
	data = NULL;

	count = 0;
	color = CV_BLACK;
	auto_color = true;
	label = "";
}

void Series::SetData(int n, float *p)
{
	Clear();

	count = n;
	data = p;
}

void Series::SetColor(int R, int G, int B, bool auto_color)
{
	R = R > 0 ? R : 0;
	G = G > 0 ? G : 0;
	B = B > 0 ? B : 0;
	color = CV_RGB(R, G, B);
	SetColor(color, auto_color);
}

void Series::SetColor(Scalar color, bool auto_color)
{
	this->color = color;
	this->auto_color = auto_color;
}

Figure::Figure(const string name)
{
	figure_name = name;

	custom_range_y = false;
	custom_range_x = false;
	backgroud_color = CV_WHITE;
	axis_color = CV_BLACK;
	text_color = CV_BLACK;

	figure_size = cvSize(600, 600);
	border_size = 30;

	tick_max = 10;

	plots.reserve(10);
}

Figure::~Figure(void)
{
}

string Figure::GetFigureName(void)
{
	return figure_name;
}

Series* Figure::Add(const Series &s)
{
	plots.push_back(s);
	return &(plots.back());
}

void Figure::Clear()
{
      plots.clear();
}

void Figure::Initialize()
{
	color_index = 0;

	// size of the figure
	if (figure_size.width <= border_size * 2 + 100)
		figure_size.width = border_size * 2 + 100;
	if (figure_size.height <= border_size * 2 + 200)
		figure_size.height = border_size * 2 + 200;

	y_max = FLT_MIN;
	y_min = FLT_MAX;

	x_max = 0;
	x_min = 0;

	// find maximum/minimum of axes
	for (auto& plot : plots)
	{
		float *p = plot.data;
		for (unsigned int i=0; i < plot.count; i++)
		{
			float v = p[i];
			if (v < y_min)
				y_min = v;
			if (v > y_max)
				y_max = v;
		}

		if (x_max < plot.count)
			x_max = (float)plot.count;
	}
	
	// find nice range and tick spacing
	float niceRange = FindNiceNum(y_max - y_min, false);
	tick_spacing = FindNiceNum(niceRange / (tick_max - 1), true);
	y_min = floor(y_min / tick_spacing) * tick_spacing;
	y_max = ceil(y_max / tick_spacing) * tick_spacing;

	// calculate zoom scale
	// set to 2 if y range is too small
	float y_range = y_max - y_min;
	float eps = 1e-9f;
	if (y_range <= eps)
	{
		y_range = 1;
		y_min = y_max / 2;
		y_max = y_max * 3 / 2;
	}

	x_scale = 1.0f;
	if (x_max - x_min > 1)
		x_scale = (float)(figure_size.width - border_size * 2) / (x_max - x_min);

	y_scale = (float)(figure_size.height - border_size * 2) / y_range;
}

float Figure::FindNiceNum(float range, bool bRound)
{
	float exponent;	 
	float fraction;	 
	float niceFraction;

	exponent = floor(log10(range));
	fraction = range / pow(10, exponent);

	if (bRound) {
		if (fraction < 1.5)
			niceFraction = 1;
		else if (fraction < 3)
			niceFraction = 2;
		else if (fraction < 7)
			niceFraction = 5;
		else
			niceFraction = 10;
	}
	else {
		if (fraction <= 1)
			niceFraction = 1;
		else if (fraction <= 2)
			niceFraction = 2;
		else if (fraction <= 5)
			niceFraction = 5;
		else
			niceFraction = 10;
	}

	return niceFraction * pow(10, exponent);
	
}

Scalar Figure::GetAutoColor()
{
	// 	change color for each curve.
	Scalar col;

	switch (color_index)
	{
	case 1:
		col = CV_RGB(60,60,255);	// light-blue
		break;
	case 2:
		col = CV_RGB(60,255,60);	// light-green
		break;
	case 3:	
		col = CV_RGB(255,60,40);	// light-red
		break;
	case 4:
		col = CV_RGB(0,210,210);	// blue-green
		break;
	case 5:
		col = CV_RGB(180,210,0);	// red-green
		break;
	case 6:
		col = CV_RGB(210,0,180);	// red-blue
		break;
	case 7:
		col = CV_RGB(0,0,185);		// dark-blue
		break;
	case 8:
		col = CV_RGB(0,185,0);		// dark-green
		break;
	case 9:
		col = CV_RGB(185,0,0);		// dark-red
		break;
	default:
		col =  CV_RGB(200,200,200);	// grey
		color_index = 0;
	}
	color_index++;
	return col;
}

void Figure::DrawAxis(Mat& output)
{
	int bs = border_size;		
	int h = figure_size.height;
	int w = figure_size.width;

	// size of graph
	int gh = h - bs * 2;
	int gw = w - bs * 2;

	// draw the horizontal and vertical axis
	// let x, y axies cross at zero if possible.
	float y_ref = y_min;
	if ((y_max > 0) && (y_min <= 0))
		y_ref = 0;

	int x_axis_pos = h - bs - cvRound((y_ref - y_min) * y_scale);

	line(output, Point(bs, x_axis_pos), Point(w - bs, x_axis_pos), axis_color);
	line(output, Point(bs, h - bs), Point(bs, h - bs - gh), axis_color);

	// Write the scale of the y axis
	//CvFont font;
	//cvInitFont(&font,CV_FONT_HERSHEY_PLAIN,0.55,0.7, 0,1,CV_AA);

	int chw = 6, chh = 12;
	char text[16];
	string text2;

	// y max
	if ((y_max - y_ref) > 0.05 * (y_max - y_min))
	{
		snprintf(text, sizeof(text)-1, "%.1f", y_max);
		putText(output, text, Point(bs / 5, bs - chh / 2), CV_FONT_HERSHEY_PLAIN, 0.9, text_color, 1, CV_AA);
	}
	// y min
	if ((y_ref - y_min) > 0.05 * (y_max - y_min))
	{
		snprintf(text, sizeof(text)-1, "%.1f", y_min);
		putText(output, text, cvPoint(bs / 5, h - bs + chh), CV_FONT_HERSHEY_PLAIN, 0.9, text_color, 1, CV_AA);
	}

	// x axis
 	snprintf(text, sizeof(text)-1, "%.1f", y_ref);
 	putText(output, text, cvPoint(0, x_axis_pos + chh / 10), CV_FONT_HERSHEY_PLAIN, 0.9, text_color, 1, CV_AA);

	// Write the scale of the x axis
	snprintf(text, sizeof(text)-1, "%.0f", x_max );
	putText(output, text, cvPoint(w - bs - strlen(text) * chw, x_axis_pos + chh), CV_FONT_HERSHEY_PLAIN, 0.9, text_color, 1, CV_AA);

	// x min
	snprintf(text, sizeof(text)-1, "%.0f", x_min );
	putText(output, text, cvPoint(bs, x_axis_pos + chh), CV_FONT_HERSHEY_PLAIN, 0.9, text_color, 1, CV_AA);


}
void Figure::DrawPlots(Mat& output)
{
	int bs = border_size;		
	int h = figure_size.height;
	int w = figure_size.width;

	// draw the curves
	for (vector<Series>::iterator iter = plots.begin();
		iter != plots.end();
		iter++)
	{
		float *p = iter->data;

		// automatically change curve color
		if (iter->auto_color == true)
			iter->SetColor(GetAutoColor());

		Point prev_point;
		for (unsigned int i=0; i<iter->count; i++)
		{
			int y = cvRound(( p[i] - y_min) * y_scale);
			int x = cvRound((   i  - x_min) * x_scale);
			Point next_point = cvPoint(bs + x, h - (bs + y));
			circle(output, next_point, 1, iter->color, 1);
			
			// draw a line between two points
			if (i >= 1)
				line(output, prev_point, next_point, iter->color, 1, CV_AA);
			prev_point = next_point;
		}
	}

}

void Figure::DrawGuidelines(Mat& output)
{
	int bs = border_size;
	int h = figure_size.height;
	int w = figure_size.width;

	// size of graph
	int gh = h - bs * 2;
	int gw = w - bs * 2;

	// draw the horizontal and vertical axis
	// let x, y axies cross at zero if possible.
	float y_ref = y_min;
	if ((y_max > 0) && (y_min <= 0))
		y_ref = 0;

	int x_axis_pos = h - bs - cvRound((y_ref - y_min) * y_scale);

	for (float tick_idx = 1; tick_idx < tick_max; tick_idx++)
	{
		int tick_pos = x_axis_pos - cvRound(tick_spacing * tick_idx * y_scale);
		line(output, Point(bs, tick_pos), Point(w - bs, tick_pos), axis_color);
	}
}

void Figure::DrawLabels(Mat& output, int posx, int posy)
{
	// character size
	int chw = 6, chh = 8;

	
	for(auto& plot : plots)
	{
		string lbl = plot.label;
		// draw label if one is available
		if (lbl.length() > 0)
		{
			line(output, Point(posx, posy - chh / 2), Point(posx + 15, posy - chh / 2), plot.color, 2, CV_AA);

			putText(output, lbl.c_str(), Point(posx + 20, posy), CV_FONT_HERSHEY_PLAIN, 1.0f, plot.color, 1, CV_AA);

			posy += int(chh * 1.5);
		}
	}

}

// whole process of draw a figure.
void Figure::Show()
{
	Initialize();

	Mat output(figure_size, CV_8UC3, backgroud_color);
	
	DrawAxis(output);
	DrawGuidelines(output);

	DrawPlots(output);

	DrawLabels(output, figure_size.width - 100, 15);

	imshow(figure_name.c_str(), output);
	waitKey(1);
}



bool PlotManager::HasFigure(string wnd)
{
	return false;	
}

// search a named window, return null if not found.
Figure* PlotManager::FindFigure(string wnd)
{
	for(vector<Figure>::iterator iter = figure_list.begin();
		iter != figure_list.end();
		iter++)
	{
		if (iter->GetFigureName() == wnd)
			return &(*iter);
	}
	return NULL;
}

// plot a new curve, if a figure of the specified figure name already exists,
// the curve will be plot on that figure; if not, a new figure will be created.
void PlotManager::Plot(const string figure_name, const float *p, int count, int step,
					   int R, int G, int B)
{
	if (count < 1)
		return;

	if (step <= 0)
		step = 1;

	// copy data and create a series format.
	float *data_copy = new float[count];

	for (int i = 0; i < count; i++)
		*(data_copy + i) = *(p + step * i);

	Series s;
	s.SetData(count, data_copy);

	if ((R > 0) || (G > 0) || (B > 0))
		s.SetColor(R, G, B, false);

	// search the named window and create one if none was found
	active_figure = FindFigure(figure_name);
	if ( active_figure == NULL)
	{
		Figure new_figure(figure_name);
		figure_list.push_back(new_figure);
		active_figure = FindFigure(figure_name);
		if (active_figure == NULL)
			exit(-1);
	}

	active_series = active_figure->Add(s);
	active_figure->Show();

}

// add a label to the most recently added curve
void PlotManager::Label(string lbl)
{
	if((active_series!=NULL) && (active_figure != NULL))
	{
		active_series->label = lbl;
		active_figure->Show();
	}
}

// plot a new curve, if a figure of the specified figure name already exists,
// the curve will be plot on that figure; if not, a new figure will be created.
// static method
template<typename T>
void plot(const string figure_name, const T* p, int count, int step,
		  int R, int G, int B)
{
	if (step <= 0)
		step = 1;

	float  *data_copy = new float[count * step];

	float   *dst = data_copy;
	const T *src = p;

	for (int i = 0; i < count * step; i++)
	{
		*dst = (float)(*src);
		dst++;
		src++;
	}

	pm.Plot(figure_name, data_copy, count, step, R, G, B);

	delete [] data_copy;
}

// delete all plots on a specified figure
void clear(const string figure_name)
{
	Figure *fig = pm.FindFigure(figure_name);
	if (fig != NULL)
	{
		fig->Clear();	
	}
	
}
// add a label to the most recently added curve
// static method
void label(string lbl)
{
	pm.Label(lbl);
}


template
void plot(const string figure_name, const unsigned char* p, int count, int step,
		  int R, int G, int B);

template
void plot(const string figure_name, const int* p, int count, int step,
		  int R, int G, int B);

template
void plot(const string figure_name, const short* p, int count, int step,
		  int R, int G, int B);

template
void plot(const string figure_name, const float* p, int count, int step,
	int R, int G, int B);

};
