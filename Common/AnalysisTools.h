#pragma once
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>

using LorentzVectorXYZ = ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>>;
using LorentzVectorM = ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>;
using LorentzVectorE = ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiE4D<double>>;
using RVecI = ROOT::VecOps::RVec<int>;
using RVecS = ROOT::VecOps::RVec<size_t>;
using RVecUC = ROOT::VecOps::RVec<UChar_t>;
using RVecF = ROOT::VecOps::RVec<float>;
using RVecB = ROOT::VecOps::RVec<bool>;
using RVecVecI = ROOT::VecOps::RVec<RVecI>;
using RVecLV = ROOT::VecOps::RVec<LorentzVectorM>;
using RVecSetInt = ROOT::VecOps::RVec<std::set<int>>;

enum class Leg : int {
  e = 1,
  mu = 2,
  tau = 3,
  jet = 4,
};
enum class Period : int {
  Run2_2016_HIPM = 1,
  Run2_2016 = 2,
  Run2_2017 = 3,
  Run2_2018 = 4,
};

enum class SampleType : int {
  GluGluToRadion = 1,
  GluGluToBulkGraviton = 2,
  VBFToRadion = 3,
  VBFToBulkGraviton = 4,
  DY = 5,
  ZQQ = 6,
  EWK = 7,
  TT = 8,
  ST = 9,
  W = 10,
  VV = 11,
  VVV = 12,
  VH = 13,
  H = 14,
  ttH = 15,
  TTV = 16,
  HHnonRes = 17,
  data = 18,
  TTT = 19,
  TTTT = 20,
  TTTV = 21,
  TTVV = 22,
  TTVH = 23,
  TTHH = 24,
  TTJ = 25,
  QCD = 26,
};


enum class GenLeptonMatch : int {
  Electron = 1,
  Muon = 2,
  TauElectron = 3,
  TauMuon = 4,
  Tau = 5,
  NoMatch = 6
};

enum class Channel : int {
  eTau = static_cast<int>(Leg::e) * 10 + static_cast<int>(Leg::tau),
  muTau = static_cast<int>(Leg::mu) * 10 + static_cast<int>(Leg::tau),
  tauTau = static_cast<int>(Leg::tau) * 10 + static_cast<int>(Leg::tau),
  eMu = static_cast<int>(Leg::e) * 10 + static_cast<int>(Leg::mu),
  eE = static_cast<int>(Leg::e) * 10 + static_cast<int>(Leg::e),
  muMu = static_cast<int>(Leg::mu) * 10 + static_cast<int>(Leg::mu)
};

inline Channel LegsToChannel(Leg leg1, Leg leg2)
{
  return static_cast<Channel>(static_cast<int>(leg1) * 10 + static_cast<int>(leg2));
}

inline std::pair<Leg, Leg> ChannelToLegs(Channel channel)
{
  const int c = static_cast<int>(channel);
  const Leg leg1 = static_cast<Leg>(c / 10);
  const Leg leg2 = static_cast<Leg>(c % 10);
  return std::make_pair(leg1, leg2);
}

template<typename T>
T DeltaPhi(T phi1, T phi2){
  return ROOT::Math::VectorUtil::Phi_mpi_pi(phi2 - phi1);
}
template<typename T>
T DeltaEta(T eta1, T eta2){
  return (eta2-eta1);
}

template<typename T>
T DeltaR(T eta1, T phi1, T eta2, T phi2){
  T dphi = DeltaPhi(phi1, phi2);
  T deta = DeltaEta(eta1, eta2);
  return std::hypot(dphi, deta);
}

RVecS CreateIndexes(size_t vecSize){
  RVecS i(vecSize);
  std::iota(i.begin(), i.end(), 0);
  return i;
}

template<typename V>
RVecI ReorderObjects(const V& varToOrder, const RVecI& indices, size_t nMax=std::numeric_limits<size_t>::max())
{
  RVecI ordered_indices = indices;
  std::sort(ordered_indices.begin(), ordered_indices.end(), [&](int a, int b) {
    return varToOrder.at(a) > varToOrder.at(b);
  });
  const size_t n = std::min(ordered_indices.size(), nMax);
  ordered_indices.resize(n);
  return ordered_indices;
}

template<typename T, int n_binary_places=std::numeric_limits<T>::digits>
std::string GetBinaryString(T x)
{
  std::bitset<n_binary_places> bs(x);
  std::ostringstream ss;
  ss << bs;
  return ss.str();
}

RVecLV GetP4(const RVecF& pt, const RVecF& eta, const RVecF& phi, const RVecF& mass, const RVecS &indices)
{
  RVecLV p4;
  p4.reserve(indices.size());
  for (auto& idx:indices)
    p4.emplace_back(pt[idx], eta[idx], phi[idx], mass[idx]);
  return p4;
}

RVecB RemoveOverlaps(const RVecLV& obj_p4, const RVecB& pre_sel, const std::vector<RVecLV>& other_objects,
                     size_t min_number_of_non_overlaps, double min_deltaR)
{
  RVecB result(pre_sel);
  const double min_deltaR2 = std::pow(min_deltaR, 2);

  const auto hasMinNumberOfNonOverlaps = [&](const LorentzVectorM& p4) {
    size_t cnt = 0;
    for(const auto& other_obj_col : other_objects) {
      for(const auto& other_obj_p4 : other_obj_col) {
        const double dR2 = ROOT::Math::VectorUtil::DeltaR2(p4, other_obj_p4);
        if(dR2 > min_deltaR2) {
          ++cnt;
          if(cnt >= min_number_of_non_overlaps)
            return true;
        }
      }
    }
    return false;
  };

  for(size_t obj_idx = 0; obj_idx < obj_p4.size(); ++obj_idx) {
    result[obj_idx] = pre_sel[obj_idx] && hasMinNumberOfNonOverlaps(obj_p4.at(obj_idx));
  }
  return result;
}

int FindMatching(const LorentzVectorM& target_p4, const RVecLV& ref_p4,const float deltaR_thr){
  double deltaR_min = deltaR_thr;
  int current_idx = -1;
  for(int refIdx =0; refIdx<ref_p4.size(); refIdx++){
    auto dR_targetRef= ROOT::Math::VectorUtil::DeltaR(target_p4, ref_p4.at(refIdx));
    if ( dR_targetRef < deltaR_min ) {
      deltaR_min = dR_targetRef ;
      current_idx = refIdx;
    }
  }
  return current_idx;
}

RVecI FindMatching(const RVecLV& target_p4, const RVecLV& ref_p4,const float deltaR_thr){
  RVecI targetIndices(target_p4.size(), -1);
  for(int targetIdx =0; targetIdx<target_p4.size(); targetIdx++){
    int refIdxFound = FindMatching(target_p4[targetIdx], ref_p4, deltaR_thr);
    targetIndices[targetIdx] = refIdxFound;
  }
  return targetIndices;
}

RVecSetInt FindMatchingSet(const RVecB& pre_sel_target, const RVecB& pre_sel_ref, const RVecLV& target_p4,
    const RVecLV& ref_p4, const float dR_thr)
    {
        RVecSetInt findMatching(pre_sel_target.size());
        for(size_t ref_idx = 0 ; ref_idx < pre_sel_ref.size() ; ref_idx ++ ){
            if(pre_sel_ref[ref_idx]==0) continue;
            for(size_t target_idx = 0 ; target_idx < pre_sel_target.size() ; target_idx ++ ){
                if(pre_sel_target[target_idx]==0) continue;
                auto dR_current = ROOT::Math::VectorUtil::DeltaR(target_p4[target_idx], ref_p4[ref_idx]);
                if(dR_current < dR_thr ){
                    findMatching[target_idx].insert(ref_idx);
                }
            }
        }
        return findMatching;
    }


namespace v_ops{
  template<typename LV>
  RVecF pt(const LV& p4){
      RVecF pt(p4.size());
      for(int p4_idx=0;p4_idx<p4.size();++p4_idx){
        pt[p4_idx] = p4.at(p4_idx).pt();
      }
      return pt;
  }
  template<typename LV>
  RVecF eta(const LV& p4){
      RVecF eta(p4.size());
      for(int p4_idx=0;p4_idx<p4.size();++p4_idx){
        eta[p4_idx] = p4.at(p4_idx).eta();
      }
      return eta;
  }
  template<typename LV>
  RVecF phi(const LV& p4){
      RVecF phi(p4.size());
      for(int p4_idx=0;p4_idx<p4.size();++p4_idx){
        phi[p4_idx] = p4.at(p4_idx).phi();
      }
      return phi;
  }
  template<typename LV>
  RVecF mass(const LV& p4){
      RVecF m(p4.size());
      for(int p4_idx=0;p4_idx<p4.size();++p4_idx){
        m[p4_idx] = p4.at(p4_idx).mass();
      }
      return m;
  }
}
