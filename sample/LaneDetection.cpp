#pragma once

// including common libraries
#include <vector>
#include <string>
#include<iostream>

// including opencv libraries
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

class LaneLineDetector {

	private:
		
	public:

		//function for changing to grayscale
		Mat change_in_grayscale(Mat image) {

			Mat gray_image;
			//using opencv to change to grayscale
			cvtColor(image, gray_image, COLOR_BGR2GRAY);

			//returning grayscale image
			return gray_image;
		}
		
		//function to clean the noise
		Mat clean_image(Mat image) {
		
			Mat cleaned_image;
			Size blur_kernel_size(5,5);
			//using gaussian blur with filter size 5 to blur the image to remove noise
			GaussianBlur(image, cleaned_image, blur_kernel_size, 0, 0);

			//returning the cleaned image
			return cleaned_image;
		}

		//function to detect the yellow and white colors
		Mat detect_yellow_white(Mat image) {
		
			//defining yellow spectrum
			Scalar yellow_low = Scalar(0, 60, 60);
			Scalar yellow_high = Scalar(30, 255, 255);

			Mat mask1, mask2, mask3;
			Mat yelwhite;
			
			//mask to filter yellow color
			inRange(image, yellow_low, yellow_high, mask1);
			
			//mask to filter white color
			inRange(image, 200, 255, mask2);

			// taking bitwise or of the both masks to get a mask with both yellow and white
			bitwise_or(mask1, mask2, mask3);

			//takin the bitwise and of mask and image to get parts of image that are corresponding to the mask
			bitwise_and(image, mask3, yelwhite);

			//returning the image containing yellow and white only
			return yelwhite;
		}
		
		//function to detect edges
		Mat detect_edges(Mat image) {

			Mat cannyed_image;

			//using canny edge detection in oencv
			Canny(image, cannyed_image, 200, 300);
			
			//returning edges
			return cannyed_image;
		}

		//function to get the region of interest
		Mat crop_image_to_roi(Mat image) {

			Mat cropped_img;

			int type_of_image = image.type();
			int height = image.size().height;
			int width = image.size().width;

			//defing the region of interest vertices
			Point vertices[3] = { Point(50, height-50), Point(width/2, height/2), Point(width-50, height-50)};
			
			//making a mask of intersted region
			Mat region_mask = Mat::zeros(Size(width,height), type_of_image);
			fillConvexPoly(region_mask, vertices, 3, Scalar(255, 0, 0));
			
			//taking bitwise and of mask iand image to get the image with region of interest points only
			bitwise_and(image, region_mask, cropped_img);

			//returning roi image
			return cropped_img;
		}

		//function the detect the lane vertices
		vector<Point> generate_laneLines(Mat image) {
			
			// final points
			vector<Point> lane_vertices(4);
			// lane points
			Point rightLane_start, leftLane_start, rightLane_end, leftLane_end;

			// height of lanes to be detected
			int lane_height = (image.size().height / 4)*3;

			// array of lines
			vector<Vec4i> lanelines, selected_lines, right_lanes, left_lanes;
			Vec4d right_lane, left_lane;

			// array of lane points
			vector<Point> right_lane_points, left_lane_points;
			
			// array of slopes
			vector<double> slopes;
			
			// slope values
			double line_slope, y1, y0, x1, x0;
			Point right_lane_bias, left_lane_bias;
			double right_lane_slope, left_lane_slope;

			// are lanes detected
			bool rightLane = false, leftLane = false;

			// hough parameters
			double rho_hough = 2, theta_hough = CV_PI / 180, min_length = 10, min_gap = 20;
			int hough_thresh = 20;

			// image mid
			int midpoint = image.size().width/2;
			
			int rightStartY = image.size().height;
			double rightStartX;
			double rightEndX;
			double leftStartX;
			double leftEndX;

			// calculating hough lines
			HoughLinesP(image, lanelines, rho_hough, theta_hough, hough_thresh, min_length, min_gap);
			
			if (!lanelines.empty()) {
				// We get the slope of lines we have detected here
				for (auto lines : lanelines) {
					rightLane_start = Point(lines[0], lines[1]);
					rightLane_end = Point(lines[2], lines[3]);

					//points to calculate the slope
					y1 = (double)(rightLane_end.y);
					y0 = (double)(rightLane_start.y);
					x1 = (double)(rightLane_end.x);
					x0 = (double)(rightLane_start.x) + 1e-6;

					// calculating slop for each line
					line_slope = (y1 - y0) / (x1 - x0);

					// calculating absoulte value of slope
					double abs_line = abs(line_slope);

					// rejecting too straight lines 
					if (abs_line > 0.3) {
						selected_lines.emplace_back(lines);
						slopes.emplace_back(line_slope);
					}

				}

				// seperating selected lines into left and right
				for (size_t size_var = 0; size_var < selected_lines.size(); size_var++) {

					int start_x = selected_lines[size_var][0];
					int start_y = selected_lines[size_var][1];
					int end_x = selected_lines[size_var][2];
					int end_y = selected_lines[size_var][3];

					rightLane_start = Point(start_x, start_y);
					rightLane_end = Point(end_x, end_y);

					if ((slopes[size_var] > 0) && (rightLane_end.x > midpoint) && (rightLane_start.x > midpoint)) {
						right_lanes.emplace_back(selected_lines[size_var]);
						rightLane = true;
					}
					else if ((slopes[size_var] < 0) && (rightLane_end.x < midpoint) && (rightLane_start.x < midpoint)) {
						left_lanes.emplace_back(selected_lines[size_var]);
						leftLane = true;
					}
				}

				// taking right and left lane points
				if (rightLane == true) {
					for (auto lane : right_lanes) {
						rightLane_start = Point(lane[0], lane[1]);
						rightLane_end = Point(lane[2], lane[3]);

						right_lane_points.emplace_back(rightLane_start);
						right_lane_points.emplace_back(rightLane_end);
					}

					if (right_lane_points.size() > 0) {
						fitLine(right_lane_points, right_lane, CV_DIST_L2, 0, 0.01, 0.01);
						right_lane_slope = right_lane[1] / right_lane[0];
						right_lane_bias = Point(right_lane[2], right_lane[3]);
					}
				}

				if (leftLane == true) {
					for (auto lane : left_lanes) {
						leftLane_start = Point(lane[0], lane[1]);
						leftLane_end = Point(lane[2], lane[3]);

						left_lane_points.emplace_back(leftLane_start);
						left_lane_points.emplace_back(leftLane_end);
					}

					if (left_lane_points.size() > 0) {
						fitLine(left_lane_points, left_lane, CV_DIST_L2, 0, 0.01, 0.01);
						left_lane_slope = left_lane[1] / left_lane[0];
						left_lane_bias = Point(left_lane[2], left_lane[3]);
					}
				}

				if (leftLane == true && rightLane == true) {
					rightStartX = ((rightStartY - right_lane_bias.y) / right_lane_slope) + right_lane_bias.x;
					rightEndX = ((lane_height - right_lane_bias.y) / right_lane_slope) + right_lane_bias.x;

					leftStartX = ((rightStartY - left_lane_bias.y) / left_lane_slope) + left_lane_bias.x;
					leftEndX = ((lane_height - left_lane_bias.y) / left_lane_slope) + left_lane_bias.x;

					lane_vertices[0] = Point(rightStartX, rightStartY);
					lane_vertices[1] = Point(rightEndX, lane_height);
					lane_vertices[2] = Point(leftStartX, rightStartY);
					lane_vertices[3] = Point(leftEndX, lane_height);

					return lane_vertices;
				}
			}

			lane_vertices[0] = Point(0, 0);
			lane_vertices[1] = Point(0, 0);
			lane_vertices[2] = Point(0, 0);
			lane_vertices[3] = Point(0, 0);

			return lane_vertices;
		}

		void drawLanes(vector<Point> lanePoints, Mat image) {
			
			vector<Point> poly_points;
			Mat drawLanePoints;

			Point line1start = lanePoints[0];
			Point line1end = lanePoints[1];
			Point line2start = lanePoints[2];
			Point line2end = lanePoints[3];

			// Make a polygon showing detected lane
			image.copyTo(drawLanePoints);
			
			poly_points.emplace_back(line1start);
			poly_points.emplace_back(line1end);
			poly_points.emplace_back(line2end);
			poly_points.emplace_back(line2start);

			Scalar laneColor = Scalar(0, 0, 0);
			Scalar lineColor = Scalar(0, 0, 255);
			int line_thickness = 7;
			
			fillConvexPoly(drawLanePoints, poly_points, laneColor, 16, 0);
			addWeighted(drawLanePoints, 0.3, image, 0.7, 0, image);

			// Plot both lines of the lane boundary
			line(image, line1start, line1end, lineColor, line_thickness, 16, 0);
			line(image, line2start, line2end, lineColor, line_thickness, 16, 0);

			// Show the final drawLanePoints image
			namedWindow("Lane Detection", CV_WINDOW_AUTOSIZE);
			imshow("Lane Detection", image);

		}

		
};