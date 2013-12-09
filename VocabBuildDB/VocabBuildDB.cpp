/*
 * VocabBuildDB.cpp
 *
 *  Created on: Oct 9, 2013
 *      Author: andresf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sys/stat.h>

#include <boost/regex.hpp>

#include <opencv2/core/core.hpp>

#include <VocabTree.h>

#include <FileUtils.hpp>

double mytime;

int main(int argc, char **argv) {

	if (argc < 5 || argc > 8) {
		printf(
				"\nUsage:\n\tVocabBuildDB <in.db.descriptors.list> "
						"<in.tree> <out.inverted.index> <out.direct.index> [in.type.binary:1]"
						" [in.use.tfidf:1] [in.normalize:1]\n\n");
		return EXIT_FAILURE;
	}

	bool use_tfidf = true;
	bool normalize = true;

	std::string list_in = argv[1];
	std::string tree_in = argv[2];
	std::string out_inv_index = argv[3];
	std::string out_dir_index = argv[4];
	bool isDescriptorBinary = true;

	if (argc >= 6) {
		isDescriptorBinary = atoi(argv[5]);
	}

	if (argc >= 7) {
		use_tfidf = atoi(argv[6]);
	}

	if (argc >= 8) {
		normalize = atoi(argv[7]);
	}

	boost::regex expression("^(.+)(\\.)(yaml|xml)(\\.)(gz)$");

	if (boost::regex_match(tree_in, expression) == false) {
		fprintf(stderr,
				"Input tree file must have the extension .yaml.gz or .xml.gz\n");
		return EXIT_FAILURE;
	}

	if (boost::regex_match(out_inv_index, expression) == false) {
		fprintf(stderr,
				"Output inverted index file must have the extension .yaml.gz or .xml.gz\n");
		return EXIT_FAILURE;
	}

	// Step 1/4: read list of descriptors that shall be used to build the tree
	printf("-- Loading list of database images descriptors\n");
	std::vector<std::string> descFilenames;
	FileUtils::loadList(list_in, descFilenames);
	printf("   Loaded, got [%lu] entries\n", descFilenames.size());

	printf("-- Building database using [%lu] images\n", descFilenames.size());

	cv::Ptr<bfeat::VocabTreeBase> tree;

	if (isDescriptorBinary == true) {
		tree = new bfeat::VocabTree<uchar, cv::Hamming>();
	} else {
		tree = new bfeat::VocabTree<float, cv::L2<float> >();
	}

	printf("-- Reading tree from [%s]\n", tree_in.c_str());

	mytime = cv::getTickCount();
	tree->load(tree_in);
	mytime = ((double) cv::getTickCount() - mytime) / cv::getTickFrequency()
			* 1000;
	printf("   Tree loaded in [%lf] ms, got [%lu] words \n", mytime,
			tree->size());

	// Step 2/4: Quantize training data (several image descriptor matrices)
	printf("-- Creating vocabulary database with [%lu] images\n",
			descFilenames.size());
	tree->clearDatabase();
	printf("   Clearing Inverted Files\n");

	cv::Mat imgDescriptors;
	uint imgIdx = 0;

	for (std::string keyFileName : descFilenames) {
		// Initialize keypoints and descriptors
		imgDescriptors = cv::Mat();
		FileUtils::loadDescriptors(keyFileName, imgDescriptors);

		// Check type of descriptors
		if ((imgDescriptors.type() == CV_8U) != isDescriptorBinary) {
			fprintf(stderr,
					"Descriptor type doesn't coincide, it is said to be [%s] while it is [%s]\n",
					isDescriptorBinary == true ? "binary" : "non-binary",
					imgDescriptors.type() == CV_8U ? "binary" : "non-binary");
			return EXIT_FAILURE;
		}

		if (imgDescriptors.rows != 0) {
			printf("   Adding image [%u] to database\n", imgIdx);
			try {
				tree->addImageToDatabase(imgIdx, imgDescriptors);
			} catch (const std::runtime_error& error) {
				fprintf(stderr, "%s\n", error.what());
				return EXIT_FAILURE;
			}
		}

		imgIdx++;
	}

	imgDescriptors.release();

	CV_Assert(descFilenames.size() == imgIdx);

	printf("   Added [%u] images\n", imgIdx);

	// Step 3/4: Compute words weights and normalize DB

	bfeat::WeightingType weightingScheme = bfeat::BINARY;
	if (use_tfidf) {
		weightingScheme = bfeat::TF_IDF;
	} else {
		weightingScheme = bfeat::BINARY;
	}

	printf("-- Computing words weights using a [%s] weighting scheme\n",
			weightingScheme == bfeat::TF_IDF ? "TF-IDF" :
			weightingScheme == bfeat::BINARY ? "BINARY" : "UNKNOWN");

	tree->computeWordsWeights(weightingScheme);

	printf("-- Applying words weights to the database BoF vectors counts\n");
	tree->createDatabase();

	int normType = cv::NORM_L1;

	if (normalize == true) {
		printf("-- Normalizing database BoF vectors using [%s]\n",
				normType == cv::NORM_L1 ? "L1-norm" :
				normType == cv::NORM_L2 ? "L2-norm" : "UNKNOWN-norm");
		tree->normalizeDatabase(normType);
	}

	printf("-- Saving inverted index to [%s]\n", out_inv_index.c_str());

	mytime = cv::getTickCount();
	tree->saveInvertedIndex(out_inv_index);
	mytime = ((double) cv::getTickCount() - mytime) / cv::getTickFrequency()
			* 1000;

	printf("   Inverted index saved in [%lf] ms\n", mytime);

	printf("-- Saving direct index to [%s]\n", out_dir_index.c_str());

	mytime = cv::getTickCount();
	tree->saveDirectIndex(out_dir_index);
	mytime = ((double) cv::getTickCount() - mytime) / cv::getTickFrequency()
			* 1000;

	printf("   Direct index saved in [%lf] ms\n", mytime);

	return EXIT_SUCCESS;
}

