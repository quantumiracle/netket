// Copyright 2018 The Simons Foundation, Inc. - All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NETKET_ADADELTA_HPP
#define NETKET_ADADELTA_HPP

#include <Eigen/Core>
#include <Eigen/Dense>
#include <cassert>
#include <cmath>
#include <complex>
#include <iostream>
#include "abstract_stepper.hpp"

namespace netket {

class AdaDelta : public AbstractStepper {
  int npar_;

  double rho_;
  double epscut_;

  Eigen::VectorXd Eg2_;
  Eigen::VectorXd Edx2_;

  int mynode_;

  const std::complex<double> I_;

 public:
  // Json constructor
  explicit AdaDelta(const json &pars)
      : rho_(FieldOrDefaultVal(pars["Learning"], "Rho", 0.95)),
        epscut_(FieldOrDefaultVal(pars["Learning"], "Epscut", 1.0e-7)),
        I_(0, 1) {
    npar_ = -1;

    PrintParameters();
  }

  void PrintParameters() {
    MPI_Comm_rank(MPI_COMM_WORLD, &mynode_);
    if (mynode_ == 0) {
      std::cout << "# Adadelta stepper initialized with these parameters : "
                << std::endl;
      std::cout << "# Rho = " << rho_ << std::endl;
      std::cout << "# Epscut = " << epscut_ << std::endl;
    }
  }

  void Init(const Eigen::VectorXd &pars) override {
    npar_ = pars.size();
    Eg2_.setZero(npar_);
    Edx2_.setZero(npar_);

  }

  void Init(const Eigen::VectorXcd &pars) override {
    npar_ = 2 * pars.size();
    Eg2_.setZero(npar_);
    Edx2_.setZero(npar_);

  }

  void Update(const Eigen::VectorXd &grad, Eigen::VectorXd &pars) override {
    assert(npar_ > 0);

    Eg2_=rho_*Eg2_+(1.-rho_)*grad.cwiseAbs2();

    Eigen::VectorXd Dx(npar_);

    for(int i=0;i<npar_;i++){
      Dx(i)=-std::sqrt(Edx2_(i)+epscut_)*grad(i);
      Dx(i)/=std::sqrt(Eg2_(i)+epscut_);
      pars(i)+=Dx(i);
    }

    Edx2_=rho_*Edx2_+(1.-rho_)*Dx.cwiseAbs2();

  }

  void Update(const Eigen::VectorXcd &grad, Eigen::VectorXd &pars) override {
    Update(Eigen::VectorXd(grad.real()), pars);
  }

  void Update(const Eigen::VectorXcd &grad, Eigen::VectorXcd &pars) override {
    assert(npar_==2*pars.size());

    Eigen::VectorXd Dx(npar_);

    for(int i=0;i<pars.size();i++){
      Eg2_(2*i)=rho_*Eg2_(2*i)+(1.-rho_)*std::pow(grad(i).real(),2);
      Eg2_(2*i+1)=rho_*Eg2_(2*i+1)+(1.-rho_)*std::pow(grad(i).imag(),2);

      Dx(2*i)=-std::sqrt(Edx2_(2*i)+epscut_)*grad(i).real();
      Dx(2*i+1)=-std::sqrt(Edx2_(2*i+1)+epscut_)*grad(i).imag();
      Dx(2*i)/=std::sqrt(Eg2_(2*i)+epscut_);
      Dx(2*i+1)/=std::sqrt(Eg2_(2*i+1)+epscut_);

      pars(i)+=Dx(2*i);
      pars(i)+=I_*Dx(2*i+1);

      Edx2_(2*i)=rho_*Edx2_(2*i)+(1.-rho_)*std::pow(Dx(2*i),2);
      Edx2_(2*i+1)=rho_*Edx2_(2*i+1)+(1.-rho_)*std::pow(Dx(2*i+1),2);
    }
  }

  void Reset() override {
    Eg2_= Eigen::VectorXd::Zero(npar_);
    Edx2_= Eigen::VectorXd::Zero(npar_);
  }

};

}  // namespace netket

#endif
