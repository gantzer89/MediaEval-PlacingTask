/*
 * CentersChooser.h
 *
 *  Created on: Sep 30, 2013
 *      Author: andresf
 */

#ifndef CENTERSCHOOSER_H_
#define CENTERSCHOOSER_H_

#include <opencv2/flann/flann.hpp>

//namespace cv {

template<typename Distance>
class CentersChooser {
public:
	virtual ~CentersChooser() {
	}
	virtual void chooseCenters(int k, int* indices, int indices_length,
			int* centers, int& centers_length, const cv::Mat& dataset,
			Distance distance = Distance()) = 0;
	static cv::Ptr<CentersChooser<Distance> > create(
			const cvflann::flann_centers_init_t& chooserType);

};

template<typename Distance>
class RandomCenters: public CentersChooser<Distance> {

	typedef typename Distance::ResultType DistanceType;

public:

	virtual ~RandomCenters() {
	}

	/**
	 * Chooses the initial centers in the k-means clustering in a random manner.
	 *
	 * @param k - Number of centers
	 * @param indices - Vector of indices in the dataset
	 * @param indices_length - Length of indices vector
	 * @param centers - Vector of cluster centers
	 * @param centers_length - Length of centers vectors
	 * @param dataset
	 * @param distance
	 */
	virtual void chooseCenters(int k, int* indices, int indices_length,
			int* centers, int& centers_length, const cv::Mat& dataset,
			Distance distance = Distance());

};

template<typename Distance>
class GonzalezCenters: public CentersChooser<Distance> {

	typedef typename Distance::ResultType DistanceType;

public:

	virtual ~GonzalezCenters() {
	}

	/**
	 * Chooses the initial centers in the k-means using Gonzalez algorithm
	 * so that the centers are spaced apart from each other.
	 *
	 * @param k - Number of centers
	 * @param indices - Vector of indices in the dataset
	 * @param indices_length - Length of indices vector
	 * @param centers - Vector of cluster centers
	 * @param centers_length - Length of centers vectors
	 * @param dataset
	 * @param distance
	 */
	virtual void chooseCenters(int k, int* indices, int indices_length,
			int* centers, int& centers_length, const cv::Mat& dataset,
			Distance distance = Distance());

};

template<typename Distance>
class KmeansppCenters: public CentersChooser<Distance> {

	typedef typename Distance::ResultType DistanceType;

public:

	virtual ~KmeansppCenters() {
	}

	/**
	 * Chooses the initial centers in the k-means using the k-means++ seeding
	 * algorithm proposed by Arthur and Vassilvitskii.
	 *
	 * @param k - Number of centers
	 * @param indices - Vector of indices in the dataset
	 * @param indices_length - Length of indices vector
	 * @param centers - Vector of cluster centers
	 * @param centers_length - Length of centers vectors
	 */
	virtual void chooseCenters(int k, int* indices, int indices_length,
			int* centers, int& centers_length, const cv::Mat& m_dataset,
			Distance distance = Distance());
};

// --------------------------------------------------------------------------

template<typename Distance>
void RandomCenters<Distance>::chooseCenters(int k, int* indices,
		int indices_length, int* centers, int& centers_length,
		const cv::Mat& dataset, Distance distance) {
	cvflann::UniqueRandom r(indices_length);

	int index;
	for (index = 0; index < k; ++index) {
		bool duplicate = true;
		int rnd;
		while (duplicate) {
			duplicate = false;
			rnd = r.next();
			if (rnd < 0) {
				centers_length = index;
				return;
			}

			centers[index] = indices[rnd];

			for (int j = 0; j < index; ++j) {
				DistanceType sq = distance(dataset.row(centers[index]).data,
						dataset.row(centers[j]).data, dataset.cols);
				if (sq < 1e-16) {
					duplicate = true;
				}
			}
		}
	}
	centers_length = index;
}

// --------------------------------------------------------------------------

template<typename Distance>
void GonzalezCenters<Distance>::chooseCenters(int k, int* indices,
		int indices_length, int* centers, int& centers_length,
		const cv::Mat& m_dataset, Distance distance) {
	int n = indices_length;

	int rnd = cvflann::rand_int(n);
	assert(rnd >= 0 && rnd < n);

	centers[0] = indices[rnd];

	int index;
	for (index = 1; index < k; ++index) {
		int best_index = -1;
		DistanceType best_val = 0;
		for (int j = 0; j < n; ++j) {
			DistanceType dist = distance(m_dataset.row(centers[0]).data,
					m_dataset.row(indices[j]).data, m_dataset.cols);
			for (int i = 1; i < index; ++i) {
				DistanceType tmp_dist = distance(m_dataset.row(centers[i]).data,
						m_dataset.row(indices[j]).data, m_dataset.cols);
				if (tmp_dist < dist) {
					dist = tmp_dist;
				}
			}
			if (dist > best_val) {
				best_val = dist;
				best_index = j;
			}
		}
		if (best_index != -1) {
			centers[index] = indices[best_index];
		} else {
			break;
		}
	}
	centers_length = index;
}

// --------------------------------------------------------------------------

template<typename Distance>
void KmeansppCenters<Distance>::chooseCenters(int k, int* indices,
		int indices_length, int* centers, int& centers_length,
		const cv::Mat& m_dataset, Distance distance) {
	int n = indices_length;

	double currentPot = 0;
	DistanceType* closestDistSq = new DistanceType[n];

	// Choose one random center and set the closestDistSq values
	int index = cvflann::rand_int(n);
	assert(index >= 0 && index < n);
	centers[0] = indices[index];

	for (int i = 0; i < n; i++) {
		closestDistSq[i] = distance(m_dataset.row(indices[i]).data,
				m_dataset.row(indices[index]).data, m_dataset.cols);
		currentPot += closestDistSq[i];
	}

	const int numLocalTries = 1;

	// Choose each center
	int centerCount;
	for (centerCount = 1; centerCount < k; centerCount++) {

		// Repeat several trials
		double bestNewPot = -1;
		int bestNewIndex = -1;
		for (int localTrial = 0; localTrial < numLocalTries; localTrial++) {

			// Choose our center - have to be slightly careful to return a valid answer even accounting
			// for possible rounding errors
			double randVal = cvflann::rand_double(currentPot);
			for (index = 0; index < n - 1; index++) {
				if (randVal <= closestDistSq[index])
					break;
				else
					randVal -= closestDistSq[index];
			}

			// Compute the new potential
			double newPot = 0;
			for (int i = 0; i < n; i++)
				newPot += std::min(
						distance(m_dataset.row(indices[i]).data,
								m_dataset.row(indices[index]).data,
								m_dataset.cols), closestDistSq[i]);

			// Store the best result
			if ((bestNewPot < 0) || (newPot < bestNewPot)) {
				bestNewPot = newPot;
				bestNewIndex = index;
			}
		}

		// Add the appropriate center
		centers[centerCount] = indices[bestNewIndex];
		currentPot = bestNewPot;
		for (int i = 0; i < n; i++)
			closestDistSq[i] = std::min(
					distance(m_dataset.row(indices[i]).data,
							m_dataset.row(indices[bestNewIndex]).data,
							m_dataset.cols), closestDistSq[i]);
	}

	centers_length = centerCount;

	delete[] closestDistSq;
}

// --------------------------------------------------------------------------

template<typename Distance>
cv::Ptr<CentersChooser<Distance> > CentersChooser<Distance>::create(
		const cvflann::flann_centers_init_t& chooserType) {

	cv::Ptr<CentersChooser<Distance> > cc;

	if (chooserType == cvflann::FLANN_CENTERS_RANDOM) {
		cc = new RandomCenters<Distance>();
	} else if (chooserType == cvflann::FLANN_CENTERS_GONZALES) {
		cc = new GonzalezCenters<Distance>();
	} else if (chooserType == cvflann::FLANN_CENTERS_KMEANSPP) {
		cc = new KmeansppCenters<Distance>();
	} else {
		CV_Error(CV_StsBadArg,
				"Unknown algorithm for choosing initial centers");
	}

	return cc;
}

//} /* namespace cv */

#endif /* CENTERSCHOOSER_H_ */