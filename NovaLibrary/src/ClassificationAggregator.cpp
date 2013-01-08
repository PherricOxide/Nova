#include "ClassificationAggregator.h"
#include "Config.h"
#include "Logger.h"

#include <stdlib.h>


using namespace std;



namespace Nova
{

ClassificationAggregator::ClassificationAggregator()
{
	Init();
}

void ClassificationAggregator::Init()
{
	vector<string> engines = Config::Inst()->GetClassificationEngines();
	vector<string> configs = Config::Inst()->GetClassifierConfigs();
	m_modes = Config::Inst()->GetClassifierModes();

	if (engines.size() != configs.size())
	{
		LOG(CRITICAL, "Invalid classification configuration!", "");
		exit(EXIT_FAILURE);
	}

	if (configs.size() != m_modes.size())
	{
		LOG(CRITICAL, "Invalid classification configuration!", "");
		exit(EXIT_FAILURE);
	}

	if (engines.size() != Config::Inst()->GetClassifierWeights().size())
	{
		LOG(CRITICAL, "Invalid classification configuration!", "");
		exit(EXIT_FAILURE);
	}

	// Compute the engine weighting
	double sum = 0;
	for (uint i = 0; i < Config::Inst()->GetClassifierWeights().size(); i++)
	{
		sum += Config::Inst()->GetClassifierWeights()[i];
	}

	for (uint i = 0; i < Config::Inst()->GetClassifierWeights().size(); i++)
	{
		m_engineWeights.push_back(Config::Inst()->GetClassifierWeights()[i]/sum);
	}


	for (uint i = 0; i < engines.size(); i++)
	{
		cout << "Loading engine " << i << " " << engines[i] << endl;
		ClassificationEngine *engine = ClassificationEngine::MakeEngine(engines[i]);
		engine->LoadConfiguration(configs[i]);

		m_engines.push_back(engine);
	}
}

double ClassificationAggregator::Classify(Suspect *s)
{
	double classification = 0;
	for (uint i = 0; i < m_engines.size(); i++)
	{
		double engineVote = m_engines.at(i)->Classify(s);
		cout << "Suspect: " << s->GetIpAddress() << " Engine: " << i << " Classification: " << engineVote << endl;

		if (m_modes[i] == CLASSIFIER_WEIGHTED)
		{
			classification += engineVote * m_engineWeights.at(i);
		}
		else if (m_modes[i] == CLASSIFIER_HOSTILE_OVERRIDE)
		{
			if (engineVote > Config::Inst()->GetClassificationThreshold())
			{
				classification = engineVote;
				break;
			}
		}
		else if (m_modes[i] == CLASSIFIER_BENIGN_OVERRIDE)
		{
			if (engineVote < Config::Inst()->GetClassificationThreshold())
			{
				classification = engineVote;
				break;
			}
		}


	}
    cout << "Suspect: " << s->GetIpAddress() << " Engine: aggrigated" << " Classification: " << classification << endl;
    cout << endl;
    s->SetClassification(classification);

	return classification;
}

} /* namespace Nova */