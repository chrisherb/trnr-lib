/*
 * dice.h
 * Copyright (c) 2025 Christopher Herb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <array>

using namespace std;

namespace trnr {
struct point {
	float x;
	float y;
};

struct rect {
	float x;
	float y;
	float width;
	float height;
};

struct quad {
	point p1, p2, p3, p4;
};

struct dice {
	int width;
	int height;
	array<point, 6> border_points;
	array<rect, 7> pips;
	array<quad, 6> segments_left;
};

// caclulates coordinates for an isometric dice control with bar segments
inline void dice_init(dice& d, float width, float height)
{
	const float shortening = 0.866f;

	float face_height, dice_width;
	face_height = dice_width = height / 2.f;
	float mid_x = width / 2.f;
	float face_half_height = face_height / 2.f;
	float face_width = dice_width * shortening;

	// border points
	point b_p1 = point {mid_x, 0};
	point b_p2 = point {mid_x + face_width, face_half_height};
	point b_p3 = point {mid_x + face_width, face_half_height + face_height};
	point b_p4 = point {mid_x, height};
	point b_p5 = point {mid_x - face_width, face_half_height + face_height};
	point b_p6 = point {mid_x - face_width, face_half_height};

	d.border_points = {b_p1, b_p2, b_p3, b_p4, b_p5, b_p6};

	const float padding = face_height * 0.09f;
	const float pad_x = padding * shortening;
	const float pad_y = padding;
	const float padded_face_height = face_height - 2 * pad_y;

	const int segments = d.segments_left.size();
	// define the ratio between segments and gaps as 3:1
	const int segment_parts = 3;
	const int gap_part = 1;
	// the number of parts per segment
	const int parts_per_segment = segment_parts + gap_part;
	const int parts = segments * parts_per_segment - 2; // remove last gap

	const float segment_height = (padded_face_height / parts) * segment_parts;
	const float gap_height = padded_face_height / parts;

	// calculate segments of the left face
	for (int i = 0; i < d.segments_left.size(); i++) {
		const point base_p1 =
			point {mid_x - face_width + pad_x, face_half_height + pad_y};
		const point base_p2 = point {mid_x - pad_x, face_height};
		float seg_y = i * (segment_height + gap_height);
		point p1 = {base_p1.x, base_p1.y + seg_y};
		point p2 = {base_p2.x, base_p2.y + seg_y};
		point p3 = {base_p2.x, base_p2.y + seg_y + segment_height};
		point p4 = {base_p1.x, base_p1.y + seg_y + segment_height};
		d.segments_left[i] = {p1, p2, p3, p4};
	}

	// calculate pip positions for top face (30-degree isometric perspective)
	// correct center of the diamond face
	// the diamond spans from b_p1 (top) to the middle of the left face
	float diamond_center_x = mid_x;
	float diamond_center_y =
		face_half_height / 2.0f + face_half_height / 2.0f; // move down to actual center

	// for 30-degree isometric, the grid directions are:
	float cos30 = 1.f;	// cos(30°) = √3/2 ≈ 0.866
	float sin30 = 0.5f; // sin(30°) = 1/2

	// scale the grid vectors to fit properly within the diamond
	float grid_scale = 0.25f; // smaller scale to fit within diamond bounds

	// grid vector 1: 30 degrees from horizontal (toward top-left of diamond)
	float grid1_x = -cos30 * face_width * grid_scale;
	float grid1_y = -sin30 * face_height * grid_scale;

	// grid vector 2: -30 degrees from horizontal (toward top-right of diamond)
	float grid2_x = cos30 * face_width * grid_scale;
	float grid2_y = -sin30 * face_height * grid_scale;

	const float pip_h = face_height * 0.1f;
	const float pip_w = pip_h * 2 * shortening;

	// position pips in the 30-degree isometric grid
	for (int i = 0; i < 7; i++) {
		float u = 0, v = 0; // grid coordinates

		switch (i) {
		case 0:
			u = 0;
			v = 0;
			break; // center
		case 1:
			u = -1;
			v = 1;
			break; // top-left
		case 2:
			u = 1;
			v = -1;
			break; // bottom-right
		case 3:
			u = 1;
			v = 1;
			break; // top-right
		case 4:
			u = -1;
			v = -1;
			break; // bottom-left
		case 5:
			u = -1;
			v = 0;
			break; // center-left
		case 6:
			u = 1;
			v = 0;
			break; // center-right
		}

		// convert grid coordinates to world coordinates using 30-degree vectors
		float pip_x = diamond_center_x + u * grid1_x + v * grid2_x;
		float pip_y = diamond_center_y + u * grid1_y + v * grid2_y;

		d.pips[i] = {pip_x - pip_w / 2, pip_y - pip_h / 2, pip_w, pip_h};
	}
}
} // namespace trnr
