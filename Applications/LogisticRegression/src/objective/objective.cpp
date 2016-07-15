#include "objective/objective.h"

#include <math.h>

#include "util/common.h"
#include "util/log.h"

#include "objective/sigmoid_objective.h"
#include "objective/softmax_objective.h"
#include "objective/ftrl_objective.h"

#include "multiverso/multiverso.h"

namespace logreg {

template<typename EleType>
Objective<EleType>::Objective(const Configure &config) {
  this->input_size_ = config.input_size;
  this->output_size_ = config.output_size;
  regular_ = new Regular<EleType>(config);
}

template<typename EleType>
Objective<EleType>::~Objective() {
  delete regular_;
}

template<typename EleType>
inline void Objective<EleType>::Gradient(Sample<EleType>* sample,
  DataBlock<EleType>*model, DataBlock<EleType>* gradient) {
  EleType* loss = new EleType[this->output_size_];
  Predict(sample, model, loss);
  Diff(sample->label, loss);
  AddRegularization(sample, model, loss, gradient);
  delete []loss;
}

template<typename EleType>
inline void Objective<EleType>::AddRegularization(Sample<EleType>*sample,
  DataBlock<EleType>* model,
  EleType* loss,
  DataBlock<EleType>* gradient) {
  if (model->sparse()) {
    size_t offset = 0;
    size_t size = sample->values.size();
    for (int i = 0; i < this->output_size_; ++i) {
      // each input
      for (size_t j = 0; j < size; ++j) {
        size_t key = sample->keys[j] + offset;
        EleType val = (EleType)(sample->values[j] * loss[i]) 
          + regular_->Calculate(key, model);
        
        EleType* pval = gradient->Get(key);
        if (pval == nullptr) {
          gradient->Set(key, val);
        } else {
          *pval += val;
        }
      }
      offset += this->input_size_;
    }
  } else {
    EleType* rawgrad = static_cast<EleType*>(gradient->raw());
    EleType* rawinput = sample->values.data();

    size_t index = 0;
    for (int i = 0; i < this->output_size_; ++i) {
      for (size_t j = 0; j < this->input_size_; ++j) {
        rawgrad[index] += (EleType)(rawinput[j] * loss[i]
          + regular_->Calculate(index, model));
        ++index;
      }
    }
  }
}

template<typename EleType>
inline void Objective<EleType>::Diff(int label, EleType*diff) {
  if (this->output_size_ == 1) {
    *diff -= static_cast<int>(label == 1);
  } else {
    for (int i = 0; i < this->output_size_; ++i) {
      diff[i] -= static_cast<int>(label == i);
    }
  }
}

template<typename EleType>
void Objective<EleType>::Predict(Sample<EleType>*sample,
  DataBlock<EleType>*model, EleType* predict) {
  for (int i = 0; i < this->output_size_; ++i) {
    predict[i] = Dot((size_t)i * this->input_size_, model, sample);
  }
}

template<typename EleType>
bool Objective<EleType>::Correct(const int label, EleType*output) {
  if (this->output_size_ == 1) {
    return (*output - static_cast<int>(label == 1)) == 0;
  }

  EleType max = *(output++);
  int idx = 0;
  for (int i = 1; i < this->output_size_; ++i) {
    if (*output > max) {
      idx = i;
      max = *output;
    }
    ++output;
  }

  return idx == label;
}

template<typename EleType>
SigmoidObjective<EleType>::SigmoidObjective(const Configure& config) :
Objective<EleType>(config) {
  if (config.output_size != 1) {
    Log::Write(Fatal, "SigmoidObjective should be used for \
                      output size = 1, with tag = [0/1]\n");
  }
}

template<typename EleType>
inline void SigmoidObjective<EleType>::Gradient(Sample<EleType>* sample,
  DataBlock<EleType>*model, DataBlock<EleType>* gradient) {
  EleType loss = (EleType)Sigmoid(sample, model);
  this->Diff(sample->label, &loss);
  this->AddRegularization(sample, model, &loss, gradient);
}

template<typename EleType>
inline void SigmoidObjective<EleType>::Predict(Sample<EleType>* sample,
  DataBlock<EleType>* model, EleType* predict) {
  *predict = Round(Sigmoid(sample, model));
}

template<typename EleType>
inline double SigmoidObjective<EleType>::Sigmoid(Sample<EleType>* sample,
  DataBlock<EleType>*model) {
  return 1.0 / (1.0 + exp(-Dot(0, model, sample)));
}

template<typename EleType>
inline EleType SigmoidObjective<EleType>::Round(double x) {
  return x < 0.5 ? (EleType)0 : (EleType)1;
}

DECLARE_TEMPLATE_CLASS_WITH_BASIC_TYPE(SigmoidObjective);

template<typename EleType>
SoftmaxObjective<EleType>::SoftmaxObjective(const Configure& config) :
Objective<EleType>(config) {
  if (config.output_size < 2) {
    Log::Write(Fatal, "SoftmaxObjective should be used for output size > 1\n");
  }
}

template<typename EleType>
inline void SoftmaxObjective<EleType>::Predict(Sample<EleType>* sample,
  DataBlock<EleType>* model, EleType* predict) {
  double sum = Sigmoid(sample, model, predict);
  for (int i = 0; i < this->output_size_; ++i) {
    predict[i] = (EleType)(predict[i] / sum);
  }
}

template<typename EleType>
double SoftmaxObjective<EleType>::Sigmoid(Sample<EleType>* sample,
  DataBlock<EleType>*model, EleType*sigmoid) {
  for (int i = 0; i < this->output_size_; ++i) {
    sigmoid[i] = Dot(i*this->input_size_, model, sample);
  }
  double max = sigmoid[0];
  for (int i = 1; i < this->output_size_; ++i) {
    max = max < sigmoid[i] ? sigmoid[i] : max;
  }
  double sum = 0.0;
  for (int i = 0; i < this->output_size_; ++i) {
    sigmoid[i] = (EleType)exp(sigmoid[i] - max);
    sum += sigmoid[i];
  }
  return sum;
}

template<typename EleType>
inline EleType SoftmaxObjective<EleType>::Round(double x) {
  return x < 0.5 ? (EleType)0 : (EleType)1;
}

DECLARE_TEMPLATE_CLASS_WITH_BASIC_TYPE(SoftmaxObjective)

template<typename EleType>
FTRLObjective<EleType>::FTRLObjective(const Configure& config) :
Objective<EleType>(config) {
  LR_CHECK(config.sparse == true);

  if (config.output_size == 1) {
    objective_ = new SigmoidObjective<EleType>(config);
  } else {
    objective_ = new SoftmaxObjective<EleType>(config);
  }
  // initiate from config
  lambda1_ = config.lambda1;
  lambda2_ = config.lambda2;
  beta_ = config.beta;
  // avoid further computing
  alpha_ = 1.0 / config.alpha;
}

template<typename EleType>
FTRLObjective<EleType>::~FTRLObjective() {
  delete objective_;
}

template<typename EleType>
void FTRLObjective<EleType>::Gradient(Sample<EleType>* sample,
  DataBlock<EleType>* model,
  DataBlock<EleType>* gradient) {
  EleType* loss = new EleType[this->output_size_];
  auto w_ = DataBlock<EleType>::GetBlock(true, model->size());

  Predict(sample, model, loss, w_);
  this->Diff(sample->label , loss);

  auto g = (DataBlock<FTRLGradient<EleType>>*)gradient;
  auto entry = (DataBlock<FTRLEntry<EleType>>*)model;
  size_t offset = 0;
  for (int i = 0; i < this->output_size_; ++i) {
    size_t size = sample->keys.size();
    for (size_t j = 0; j < size; ++j) {
      double delta_z;

      double delta_g = sample->values[j] * loss[i];
      double square_g = delta_g * delta_g;

      size_t key = sample->keys[j] + offset;
      EleType *w = w_->Get(key);
      if (w == nullptr) {
        delta_z = -delta_g;
      } else {
        FTRLEntry<EleType> *en = entry->Get(key);
        if (en == nullptr) {
          delta_z = alpha_ * delta_g;
        } else {
          delta_z = alpha_ * (sqrt(en->n + square_g) - en->sqrtn);
        }
        delta_z = delta_z * (*w) - delta_g;
      }
      // delta_n
      delta_g = -square_g;
      g->Set(key, FTRLGradient<EleType>((EleType)delta_z, (EleType)delta_g));
    }
    offset += this->input_size_;
  }
  delete[]loss;
  delete w_;
}

template<typename EleType>
void FTRLObjective<EleType>::Predict(Sample<EleType>* sample,
  DataBlock<EleType>* model, EleType* predict) {
  auto w = DataBlock<EleType>::GetBlock(true, model->size());
  Predict(sample, model, predict, w);
  delete w;
}

template<typename EleType>
void FTRLObjective<EleType>::Predict(Sample<EleType>*sample,
  DataBlock<EleType>* model, EleType* predict, DataBlock<EleType>* w) {
  auto entry = (DataBlock<FTRLEntry<EleType>>*)model;
  w->Clear();

  size_t offset = 0;
  for (size_t i = 0; i < this->output_size_; ++i) {
    for (size_t j = 0; j < sample->values.size(); ++j) {
      FTRLEntry<EleType> *en = entry->Get(sample->keys[j] + offset);
      if (en != nullptr && abs(en->z) > lambda1_) {
        EleType val = (EleType)((sgn(en->z) * lambda1_ - en->z)
          / ((beta_ + en->sqrtn) * alpha_ + lambda2_));
        w->Set(sample->keys[j] + offset, val);
      }
    }
    offset += this->input_size_;
  }

  objective_->Predict(sample, w, predict);
}

template<typename EleType>
EleType FTRLObjective<EleType>::sgn(const EleType x) {
  return (EleType)(x > 0 ? 1 : (x < 0 ? -1 : 0));
}

DECLARE_TEMPLATE_CLASS_WITH_BASIC_TYPE(FTRLObjective);

template<typename EleType>
Objective<EleType>* Objective<EleType>::Get(
  const Configure &config) {
  const std::string& type = config.objective_type;
  Log::Write(Info, "Objective type %s\n", type.c_str());

  if (type == "sigmoid") {
    return new SigmoidObjective<EleType>(config);
  } else if (type == "softmax") {
    return new SoftmaxObjective<EleType>(config);
  } else if (type == "ftrl") {
    return new FTRLObjective<EleType>(config);
  }

  return new Objective<EleType>(config);
}

DECLARE_TEMPLATE_CLASS_WITH_BASIC_TYPE(Objective);
}  // namespace logreg 
