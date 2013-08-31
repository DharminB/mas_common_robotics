/*
 * This file is part of MultiBoost, a multi-class
 * AdaBoost learner/classifier
 *
 * Copyright (C) 2005-2006 Norman Casagrande
 * For informations write to nova77@gmail.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "weak_learners/haar_multi_stump_learner.h"

#include "io/haar_data.h"
#include "io/serialization.h"
#include "others/haar_features.h" // for shortname->type and viceversa (see serialization)
#include <limits> // for numeric_limits
#include <ctime> // for time
namespace MultiBoost
{

	REGISTER_LEARNER_NAME(HaarMultiStump, HaarMultiStumpLearner)

// ------------------------------------------------------------------------------

	void HaarMultiStumpLearner::declareArguments(nor_utils::Args& args)
	{
		// call the superclasses
		HaarLearner::declareArguments(args);
		MultiStumpLearner::declareArguments(args);
	}

// ------------------------------------------------------------------------------

	void HaarMultiStumpLearner::initOptions(nor_utils::Args& args)
	{
		// call the superclasses
		HaarLearner::initOptions(args);
		MultiStumpLearner::initOptions(args);
	}

// ------------------------------------------------------------------------------

	double HaarMultiStumpLearner::classify(InputData* pData, int idx, int classIdx)
	{
		// The integral image data from the input must be transformed into the
		// feature's space. This is done by getValue of the selected feature.
		return _v[classIdx] * MultiStumpLearner::phi(_pSelectedFeature->getValue(static_cast<HaarData*>(pData)->getIntImage(idx), _selectedConfig), classIdx);

	}

// ------------------------------------------------------------------------------

	void HaarMultiStumpLearner::run(InputData* pData)
	{
		const int numClasses = ClassMappings::getNumClasses();

		// set the smoothing value to avoid numerical problem
		// when theta=0.
		setSmoothingVal(1.0 / (double) pData->getNumExamples() * 0.01);

		// resize: it's done here to avoid a reallocation
		// for each feature.
		_leftErrors.resize(numClasses);
		_rightErrors.resize(numClasses);
		_bestErrors.resize(numClasses);
		_weightsPerClass.resize(numClasses);
		_halfWeightsPerClass.resize(numClasses);

		vector<sRates> mu(numClasses);  // The class-wise rates. See BaseLearner::sRates for more info.
		vector<double> tmpV(numClasses);  // The class-wise votes/abstentions

		vector<double> tmpThresholds(numClasses);
		double tmpAlpha;

		double bestE = numeric_limits<double>::max();
		double tmpE;

		HaarData* pHaarData = static_cast<HaarData*>(pData);

		// get the whole data matrix
		const vector<int*>& intImages = pHaarData->getIntImageVector();

		// The data matrix transformed into the feature's space
		vector<pair<int, int> > processedHaarData(intImages.size());

		// I need to prepare both type of sampling
		int numConf;  // for ST_NUM
		time_t startTime, currentTime;  // for ST_TIME

		long numProcessed;
		bool quitConfiguration;

		// The declared features types
		vector<HaarFeature*>& loadedFeatures = pHaarData->getLoadedFeatures();

		// for every feature type
		vector<HaarFeature*>::iterator ftIt;
		for (ftIt = loadedFeatures.begin(); ftIt != loadedFeatures.end(); ++ftIt)
		{
			// just for readability
			HaarFeature* pCurrFeature = *ftIt;
			if (_samplingType != ST_NO_SAMPLING)
				pCurrFeature->setAccessType(AT_RANDOM_SAMPLING);

			// Reset the iterator on the configurations. For random sampling
			// this shuffles the configurations.
			pCurrFeature->resetConfigIterator();
			quitConfiguration = false;
			numProcessed = 0;

			numConf = 0;
			time(&startTime);

			if (_verbose > 1)
				cout << "Learning type " << pCurrFeature->getName() << ".." << flush;

			// While there is a configuration available
			while (pCurrFeature->hasConfigs())
			{
				// transform the data from intImages to the feature's space
				pCurrFeature->fillHaarData(intImages, processedHaarData);
				// sort the examples in the new space by their coordinate
				sort(processedHaarData.begin(), processedHaarData.end(), nor_utils::comparePairOnSecond<int, int, less<int> >);

				// find the optimal threshold
				findThreshold<int>(processedHaarData.begin(), processedHaarData.end(), pData, tmpThresholds, mu, tmpV);

				tmpE = getEnergy(mu, tmpAlpha, tmpV);
				++numProcessed;

				if (tmpE < bestE)
				{
					// Store it in the current weak hypothesis.
					// note: I don't really like having so many temp variables
					// but the alternative would be a structure, which would need
					// to be inheritable to make things more consistent. But this would
					// make it less flexible. Therefore, I am still undecided. This
					// might change!
					_alpha = tmpAlpha;
					_v = tmpV;

					// I need to save the configuration because it changes within the object
					_selectedConfig = pCurrFeature->getCurrentConfig();
					// I save the object because it contains the informations about the type,
					// the name, etc..
					_pSelectedFeature = pCurrFeature;
					_thresholds = tmpThresholds;

					bestE = tmpE;
				}

				// Move to the next configuration
				pCurrFeature->moveToNextConfig();

				// check stopping criterion for random configurations
				switch (_samplingType)
				{
					case ST_NUM:
						++numConf;
						if (numConf >= _samplingVal)
							quitConfiguration = true;
						break;
					case ST_TIME:
						time(&currentTime);
						double diff = difftime(currentTime, startTime);  // difftime is in seconds
						if (diff >= _samplingVal)
							quitConfiguration = true;
						break;

				}  // end switch

				if (quitConfiguration)
					break;

			}  // end while

			if (_verbose > 1)
			{
				time(&currentTime);
				double diff = difftime(currentTime, startTime);  // difftime is in seconds

				cout << "done! " << "(processed: " << numProcessed << " - elapsed: " << diff << " sec)" << endl;
			}

		}

		if (!_pSelectedFeature)
		{
			cerr << "ERROR: No Haar Feature found. Something must be wrong!" << endl;
			exit(1);
		}
		else
		{
			if (_verbose > 1)
				cout << "Selected type: " << _pSelectedFeature->getName() << endl;
		}

	}

// ------------------------------------------------------------------------------

	InputData* HaarMultiStumpLearner::createInputData()
	{
		return new HaarData();
	}

// ------------------------------------------------------------------------------

	void HaarMultiStumpLearner::save(ofstream& outputStream, int numTabs)
	{
		// Calling the super-class methods
		MultiStumpLearner::save(outputStream, numTabs);
		HaarLearner::save(outputStream, numTabs);
	}

// -----------------------------------------------------------------------

	void HaarMultiStumpLearner::load(nor_utils::StreamTokenizer& st)
	{
		// Calling the super-class methods
		MultiStumpLearner::load(st);
		HaarLearner::load(st);
	}

// -----------------------------------------------------------------------

}// end of MultiBoost namespace
