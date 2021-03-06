/*
 * DBriefDescriptorExtractor.cpp
 *
 *  Created on: Aug 22, 2013
 *      Author: andresf
 */

#include <DBriefDescriptorExtractor.h>
#include <iostream>
#include <string>

namespace cv {

void DBrief(const Mat& image, vector<KeyPoint>& keypoints, Mat& descriptors,
		const int& size, const int& type) {

	// Converting color image to grayscale
	Mat grayImage = image;
	if (image.type() != CV_8U)
		cvtColor(image, grayImage, COLOR_BGR2GRAY);

	// DBrief object which contains methods describing keypoints with dbrief descriptors.
	CVLAB::Dbrief dbrief;

	IplImage ipl_grayImage = grayImage;

	vector<bitset<CVLAB::DESC_LEN> > descs;
	dbrief.getDbriefDescriptors(descs, keypoints, &ipl_grayImage);

	descriptors.create((int) keypoints.size(), size, type);
	// Initializing matrix with zeros
	std::fill_n(descriptors.data, descriptors.rows * descriptors.cols, 0);

	for (int i = 0; i < (int) descs.size(); i++) {
		bitset<CVLAB::DESC_LEN> desc = descs[i];
		//printf("String formatted: %s\n", desc.to_string<char, char_traits<char>, allocator<char> >().c_str());

		// Doing a reverse loop because the most significant bit on a bitset is the last element
		// e.g. 1100 is stored as 0011
		for (int j = (int) desc.size() - 1; j >= 0; j--) {
			// Incrementally composing descriptor by
			// splitting the 32 length bitset in chunks of 8 bits (1 byte)
			// and converting it into its uint representation
			descriptors.at<unsigned char>(i, descriptors.cols - 1 - (int) j / 8) +=
					(desc.test(j) << (j % 8));
		}
	}

}

DBriefDescriptorExtractor::DBriefDescriptorExtractor() {
	;
}

DBriefDescriptorExtractor::~DBriefDescriptorExtractor() {
	;
}

int DBriefDescriptorExtractor::descriptorSize() const {
	// D-Brief descriptor has length 32 bits or 4 Bytes
	return CVLAB::DESC_LEN / 8;
}

int DBriefDescriptorExtractor::descriptorType() const {
	return CV_8U;
}

void DBriefDescriptorExtractor::computeImpl(const Mat& image,
		std::vector<KeyPoint>& keypoints, Mat& descriptors) const {

	KeyPointsFilter::runByImageBorder(keypoints, image.size(),
			CVLAB::IMAGE_PADDING_TOP);
	DBrief(image, keypoints, descriptors, this->descriptorSize(),
			this->descriptorType());
}

} /* namespace cv */
