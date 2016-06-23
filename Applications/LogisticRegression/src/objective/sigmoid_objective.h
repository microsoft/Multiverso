#ifndef LOGREG_OBJECTIVE_SIGMOID_OBJECTIVE_H_
#define LOGREG_OBJECTIVE_SIGMOID_OBJECTIVE_H_

#include "objective.h"

namespace logreg {

template<typename EleType>
class SigmoidObjective : public Objective<EleType> {
public:
  explicit SigmoidObjective(const Configure& config);

  void Gradient(Sample<EleType>* sample,
    DataBlock<EleType>* model,
    DataBlock<EleType>* gradient);

  void Predict(Sample<EleType>*sample,
    DataBlock<EleType>* model, EleType* predict);

private:
  double Sigmoid(Sample<EleType>* sample,
    DataBlock<EleType>*model);
  EleType Round(double x);
};

}  // namespace logreg

#endif  // LOGREG_OBJECTIVE_SIGMOID_OBJECTIVE_H_
