/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef THREE_GPP_SPECTRUM_PROPAGATION_LOSS_H
#define THREE_GPP_SPECTRUM_PROPAGATION_LOSS_H

#include "ns3/spectrum-propagation-loss-model.h"
#include <complex.h>
#include <map>
#include <unordered_map>
#include "ns3/matrix-based-channel-model.h"

// ADDED DURING MERGING
#include <fstream>
#include <ns3/output-stream-wrapper.h>
#include <ns3/net-device.h>
#include <ns3/three-gpp-channel-model.h>
//#include <ns3/ideal-beamforming-helper.h>

namespace ns3 {

class NetDevice;
class ChannelConditionModel;
class ChannelCondition;

struct LinkData;
struct RxRayData;
struct EnbTraceData;


/**
 * \ingroup spectrum
 * \brief 3GPP Spectrum Propagation Loss Model
 *
 * This class models the frequency dependent propagation phenomena in the way
 * described by 3GPP TR 38.901 document. The main method is DoCalcRxPowerSpectralDensity,
 * which takes as input the power spectral density (PSD) of the transmitted signal,
 * the mobility models of the transmitting node and receiving node, and
 * returns the PSD of the received signal.
 *
 * \see MatrixBasedChannelModel
 * \see ThreeGppAntennaArrayModel
 * \see ChannelCondition
 */
class ThreeGppSpectrumPropagationLossModel : public SpectrumPropagationLossModel
{
public:
  /**
   * Constructor
   */
  ThreeGppSpectrumPropagationLossModel ();

  /**
   * Destructor
   */
  ~ThreeGppSpectrumPropagationLossModel ();
  
  void DoDispose () override;

  /**
   * Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  /**
   * Set the channel model object
   * \param channel a pointer to an object implementing the MatrixBasedChannelModel interface
   */
  void SetChannelModel (Ptr<MatrixBasedChannelModel> channel);

  /**
   * Get the channel model object
   * \return a pointer to the object implementing the MatrixBasedChannelModel interface
   */
  Ptr<MatrixBasedChannelModel> GetChannelModel() const;

  /**
   * Add a device-antenna pair
   * \param n a pointer to the NetDevice
   * \param a a pointer to the associated ThreeGppAntennaArrayModel
   */
  void AddDevice (Ptr<NetDevice> n, Ptr<const ThreeGppAntennaArrayModel> a);


  /**
   * Sets the value of an attribute belonging to the associated
   * MatrixBasedChannelModel instance
   * \param name name of the attribute
   * \param value the attribute value
   */
  void SetChannelModelAttribute (const std::string &name, const AttributeValue &value);

  /**
   * Returns the value of an attribute belonging to the associated
   * MatrixBasedChannelModel instance
   * \param name name of the attribute
   * \param where the result should be stored
   */
  void GetChannelModelAttribute (const std::string &name, AttributeValue &value) const;

  /**
   * \brief Computes the received PSD.
   *
   * This function computes the received PSD by applying the 3GPP fast fading
   * model and the beamforming gain.
   * In particular, it retrieves the matrix representing the channel between
   * node a and node b, computes the corresponding long term component, i.e.,
   * the product between the cluster matrices and the TX and RX beamforming
   * vectors (w_rx^T H^n_ab w_tx), and accounts for the Doppler component and
   * the propagation delay.
   * To reduce the computational load, the long term component associated with
   * a certain channel is cached and recomputed only when the channel realization
   * is updated, or when the beamforming vectors change.
   *
   * \param txPsd tx PSD
   * \param a first node mobility model
   * \param b second node mobility model
   *
   * \return the received PSD
   */
  Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                           Ptr<const MobilityModel> a,
                                                           Ptr<const MobilityModel> b) const override;

  // ADDED DURING MERGING

  void LoadAllTraceData(uint16_t ueWalkId);

  //void SetSMIdealBeamformingHelper (Ptr<IdealBeamformingAlgorithm> idealBeamformingHelper);

  Vector GetCurrentUePosition();

  /**
   * Computes the long term component
   * \param channelMatrix the channel matrix H
   * \param sW the beamforming vector of the s device
   * \param uW the beamforming vector of the u device
   * \return the long term component
   */
  ThreeGppAntennaArrayModel::ComplexVector CalcLongTerm (Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
                                                         const ThreeGppAntennaArrayModel::ComplexVector &sW,
                                                         const ThreeGppAntennaArrayModel::ComplexVector &uW) const;

  /**
   * Computes the beamforming gain and applies it to the tx PSD
   * \param txPsd the tx PSD
   * \param longTerm the long term component
   * \param params The channel matrix
   * \param speed speed of the moving node
   * \return the rx PSD
   */
  Ptr<SpectrumValue> CalcBeamformingGain (Ptr<SpectrumValue> txPsd,
                                          ThreeGppAntennaArrayModel::ComplexVector longTerm,
                                          ThreeGppAntennaArrayModel::ComplexVector txW,
                                          ThreeGppAntennaArrayModel::ComplexVector rxW,
                                          Ptr<const MatrixBasedChannelModel::ChannelMatrix> params,
                                          double speed) const;

  void SetBeamSweepState (bool beamSweepState);

  std::pair<std::pair<double, double>, std::pair<double, double>> GetAodAoaFromCM (Vector enbLoc);

private:
  /**
   * Data structure that stores the long term component for a tx-rx pair
   */
  struct LongTerm : public SimpleRefCount<LongTerm>
  {
    ThreeGppAntennaArrayModel::ComplexVector m_longTerm; //!< vector containing the long term component for each cluster
    Ptr<const MatrixBasedChannelModel::ChannelMatrix> m_channel; //!< pointer to the channel matrix used to compute the long term
    ThreeGppAntennaArrayModel::ComplexVector m_sW; //!< the beamforming vector for the node s used to compute the long term
    ThreeGppAntennaArrayModel::ComplexVector m_uW; //!< the beamforming vector for the node u used to compute the long term
  };

  /**
   * Get the operating frequency
   * \return the operating frequency in Hz
  */
  double GetFrequency () const;

  /**
   * Looks for the long term component in m_longTermMap. If found, checks
   * whether it has to be updated. If not found or if it has to be updated,
   * calls the method CalcLongTerm to compute it.
   * \param aId id of the first node
   * \param bId id of the second node
   * \param channelMatrix the channel matrix
   * \param aW the beamforming vector of the first device
   * \param bW the beamforming vector of the second device
   * \return vector containing the long term compoenent for each cluster
   */
  ThreeGppAntennaArrayModel::ComplexVector GetLongTerm (uint32_t aId, uint32_t bId,
                                                        Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
                                                        const ThreeGppAntennaArrayModel::ComplexVector &aW,
                                                        const ThreeGppAntennaArrayModel::ComplexVector &bW) const;
    
  /**
   * Computes the beamforming gain and applies it to the tx PSD
   * \param txPsd the tx PSD
   * \param longTerm the long term component
   * \param params The channel matrix
   * \param sSpeed speed of the first node
   * \param uSpeed speed of the second node
   * \return the rx PSD
   */
  Ptr<SpectrumValue> CalcBeamformingGain (Ptr<SpectrumValue> txPsd,
                                          ThreeGppAntennaArrayModel::ComplexVector longTerm,
                                          Ptr<const MatrixBasedChannelModel::ChannelMatrix> params,
                                          const Vector &sSpeed, const Vector &uSpeed) const;

  std::unordered_map <uint32_t, Ptr<const ThreeGppAntennaArrayModel> > m_deviceAntennaMap; //!< map containig the <node, antenna> associations
  mutable std::unordered_map < uint32_t, Ptr<const LongTerm> > m_longTermMap; //!< map containing the long term components
  Ptr<MatrixBasedChannelModel> m_channelModel; //!< the model to generate the channel matrix

  // ADDED DURING MERGING

  std::string input_files_folder;
  mutable std::vector<Vector> enbLocations; // locations of all the ENB
  std::vector<Vector> walkCords;
  std::vector<double> walkSpeed;
  std::vector<std::vector<int> >  los_data;
  std::vector<std::vector<int> >  link_data;
  mutable std::map<Vector, Ptr<LinkData>> allLinkData;
  mutable std::map<Vector, Ptr<EnbTraceData>> all_enbTraceData; 
  bool enablePedsBlockage; // enable pedestrian blockage
  mutable Ptr<OutputStreamWrapper> stream;
  uint32_t maxTraceIndexSM;
  bool channelTypeRaytracingSM; // channelTypeRaytracing for ThreeGppSpectrumPropagationLossModel
  //Ptr<IdealBeamformingHelper> m_SMidealBeamformingHelper;    //Set from Nr-Gnb-Phy

  void LoadEnbLocations (std::string input_folder);
  void LoadEnbTraceData (Vector enbLocation);
  void LoadWalkInfo (std::string walkInfoFile, std::string speedFile);
  void LoadLosInfo (std::string los_file);
  std::string GetTraceFileName(Vector enbLoc);
  void LoadInventory (std::string inventory_file);
  void LoadLinkInfo (std::string link_file);

  /**
   * @brief * Scan all sectors with predefined code book and select the one returns maximum gain.
   * The BF vector is stored in the Params3gpp object passed as parameter
   * @param aAntenna Antenna array of the transmitter
   * @param bAntenna Antenna array of the receiver
   */
  void BeamSearchBeamforming (Ptr<const ThreeGppAntennaArrayModel> aAntenna,
                              Ptr<const ThreeGppAntennaArrayModel> bAntenna,
                              Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix) const;

  void LongTermCovMatrixBeamforming (Ptr<const ThreeGppAntennaArrayModel> aAntenna,
                                     Ptr<const ThreeGppAntennaArrayModel> bAntenna,
                                     Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix) const;

  // ADDED DURING MERGING
  void SMSetRaySourceType (std::string type);
  std::string SMGetRaySourceType ();
  std::string m_smRaySourceType;
  std::string m_frequency;
};
} // namespace ns3

#endif /* THREE_GPP_SPECTRUM_PROPAGATION_LOSS_H */
