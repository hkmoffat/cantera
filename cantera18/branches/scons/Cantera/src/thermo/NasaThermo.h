/**
 * @file NasaThermo.h
 *   Header for the 2 regime 7 coefficient Nasa thermodynamic
 *   polynomials for multiple species in a phase, derived from the
 *   \link Cantera::SpeciesThermo SpeciesThermo\endlink base class (see \ref mgrsrefcalc and
 *   \link Cantera::NasaThermo NasaThermo\endlink).
 */
// Copyright 2003 California Institute of Technology

#ifndef CT_NASATHERMO_H
#define CT_NASATHERMO_H
#include <string>

#include "SpeciesThermoMgr.h"
#include "NasaPoly1.h"
#include "speciesThermoTypes.h"
//#include "polyfit.h"
#include "global.h"

namespace Cantera {

  /**
   * A species thermodynamic property manager for the NASA
   * polynomial parameterization with two temperature ranges.
   *
   * This class is designed to efficiently evaluate the properties
   * of a large number of species with the NASA parameterization.
   *
   * The original NASA polynomial parameterization expressed the
   * heat capacity as a fourth-order polynomial in temperature, with
   * separate coefficients for each of two temperature ranges. (The
   * newer NASA format adds coefficients for 1/T and 1/T^2, and
   * allows multiple temperature ranges.) This class is designed for
   * use with the original parameterization, which is used, for
   * example, by the Chemkin software package.
   * 
   * In many cases, the midpoint temperature is the same for many
   * species.  To take advantage of this, class NasaThermo groups
   * species with a common midpoint temperature, so that checking
   * which range the desired temperature is in need be done only
   * once for each group.
   *
   * @note There is a special CTML element for entering the 
   * coefficients of this parameterization.
   * @see importCTML
   *
   * @ingroup mgrsrefcalc
   */
  class NasaThermo : public SpeciesThermo {
    
  public:

    //! Initialized to the type of parameterization
    /*!
     * Note, this value is used in some template functions
     */
    const int ID;

    //! constructor
    NasaThermo() :
      ID(NASA),  
      m_tlow_max(0.0), 
      m_thigh_min(1.e30),
      m_p0(-1.0),
      m_ngroups(0)
    {
      m_t.resize(6);
    }

    //! Copy constructor
    /*!
     *  @param right NasaThermo object to be copied.
     */
    NasaThermo(const NasaThermo &right) :
      ID(NASA),  
      m_tlow_max(0.0), 
      m_thigh_min(1.e30),
      m_p0(-1.0),
      m_ngroups(0)
    {
      *this = operator=(right);
    }

    //! Assignment operator
    /*!
     *  @param right NasaThermo object to be copied.
     */
    NasaThermo& operator=(const NasaThermo &right) {
      /*
       * Check for self assignment.
       */
      if (this == &right) return *this;

      m_high           = right.m_high;
      m_low            = right.m_low;
      m_index          = right.m_index;
      m_tmid           = right.m_tmid;
      m_tlow_max       = right.m_tlow_max;
      m_thigh_min      = right.m_thigh_min;
      m_tlow           = right.m_tlow;
      m_thigh          = right.m_thigh;
      m_p0             = right.m_p0;
      m_ngroups        = right.m_ngroups;
      m_t              = right.m_t;
      m_group_map      = right.m_group_map;
      m_posInGroup_map = right.m_posInGroup_map;
      m_name           = right.m_name;

      return *this;
    }


    //! destructor
    virtual ~NasaThermo() {}

    //! Duplication routine for objects which inherit from 
    //! %SpeciesThermo
    /*!
     *  This virtual routine can be used to duplicate %SpeciesThermo  objects
     *  inherited from %SpeciesThermo even if the application only has
     *  a pointer to %SpeciesThermo to work with.
     *  ->commented out because we first need to add copy constructors
     *   and assignment operators to all of the derived classes.
     */
    virtual SpeciesThermo *duplMyselfAsSpeciesThermo() const {
      NasaThermo *nt = new NasaThermo(*this);
      return (SpeciesThermo *) nt;
    }

    //! install a new species thermodynamic property
    //!  parameterization for one species.  
    /*!
     *
     * @param name      Name of the species
     * @param index     The 'update' method will update the property 
     *                  values for this species 
     *                  at position i index in the property arrays.  
     * @param type      int flag specifying the type of parameterization to be
     *                 installed. 
     * @param c        vector of coefficients for the parameterization. 
     * - c[0]          midpoint temperature
     * - c[1] - c[7]   coefficients for low T range
     * - c[8] - c[14]  coefficients for high T range
     * @param minTemp  minimum temperature for which this parameterization
     *                 is valid.
     * @param maxTemp  maximum temperature for which this parameterization
     *                 is valid.
     * @param refPressure standard-state pressure for this 
     *                    parameterization. 
     * @see speciesThermoTypes.h 
     */
    virtual void install(string name, size_t index, int type,
			 const doublereal* c, 
			 doublereal minTemp, doublereal maxTemp,
			 doublereal refPressure) { 

      m_name[index] = name;
      int imid = int(c[0]);       // midpoint temp converted to integer
      int igrp = m_index[imid];   // has this value been seen before?
      if (igrp == 0) {            // if not, prepare new group
	vector<NasaPoly1> v;
	m_high.push_back(v);
	m_low.push_back(v);
	m_tmid.push_back(c[0]);
	m_index[imid] = igrp = static_cast<int>(m_high.size());
	m_ngroups++;
      }

      m_group_map[index] = igrp;
      m_posInGroup_map[index] = (int) m_low[igrp-1].size();
	    
      doublereal tlow  = minTemp;
      doublereal tmid  = c[0];
      doublereal thigh = maxTemp;
      const doublereal* clow = c + 1;

      vector_fp chigh(7);
      copy(c + 8, c + 15, chigh.begin());

      m_high[igrp-1].push_back(NasaPoly1(index, tmid, thigh, 
                                         refPressure, &chigh[0]));
      m_low[igrp-1].push_back(NasaPoly1(index, tlow, tmid, 
                                        refPressure, clow));

      vector_fp clu(7), chu(7);
      clu[5] = clow[0];
      clu[6] = clow[1];
      copy(clow+2, clow+7, clu.begin());
      chu[5] = chigh[0];
      chu[6] = chigh[1];
      copy(chigh.begin()+2, chigh.begin()+7, chu.begin());

      checkContinuity(name, tmid, &clu[0], &chu[0]);

      if (tlow > m_tlow_max)    m_tlow_max = tlow;
      if (thigh < m_thigh_min)  m_thigh_min = thigh;
      if (m_tlow.size() < index + 1) {
	m_tlow.resize(index + 1,  tlow);
	m_thigh.resize(index + 1, thigh);
      }
      m_tlow[index] = tlow;
      m_thigh[index] = thigh;
      if (m_p0 < 0.0) {
	m_p0 = refPressure;
      } else if (fabs(m_p0 - refPressure) > 0.1) {
	string logmsg =  " WARNING NasaThermo: New Species, " + name +  ", has a different reference pressure, "
	  + fp2str(refPressure) + ", than existing reference pressure, " 	+ fp2str(m_p0) + "\n";
	writelog(logmsg);
	logmsg = "                  This may become a fatal error in the future \n";
	writelog(logmsg);
      }
      m_p0 = refPressure;
    }

    //! Install a new species thermodynamic property
    //! parameterization for one species.
    /*!
     * @param stit_ptr Pointer to the SpeciesThermoInterpType object
     *          This will set up the thermo for one species
     */
    virtual void install_STIT(SpeciesThermoInterpType *stit_ptr) {
      throw CanteraError("install_STIT", "not implemented");
    }
 

    //! Like update(), but only updates the single species k.
    /*!
     * @param k       species index
     * @param t       Temperature (Kelvin)
     * @param cp_R    Vector of Dimensionless heat capacities.
     *                (length m_kk).
     * @param h_RT    Vector of Dimensionless enthalpies.
     *                (length m_kk).
     * @param s_R     Vector of Dimensionless entropies.
     *                (length m_kk).
     *
     */
    virtual void update_one(size_t k, doublereal t, doublereal* cp_R,
			    doublereal* h_RT, doublereal* s_R) const {

      m_t[0] = t;
      m_t[1] = t*t;
      m_t[2] = m_t[1]*t;
      m_t[3] = m_t[2]*t;
      m_t[4] = 1.0/t;
      m_t[5] = log(t);
 
      size_t grp = m_group_map[k];
      size_t pos = m_posInGroup_map[k];
      const vector<NasaPoly1> &mlg = m_low[grp-1];
      const NasaPoly1 *nlow = &(mlg[pos]);

      doublereal tmid = nlow->maxTemp();
      if (t < tmid) {
	nlow->updateProperties(&m_t[0], cp_R, h_RT, s_R);
      } else {
	const vector<NasaPoly1> &mhg = m_high[grp-1];
	const NasaPoly1 *nhigh = &(mhg[pos]);
	nhigh->updateProperties(&m_t[0], cp_R, h_RT, s_R);
      }
    }

    //! Compute the reference-state properties for all species.
    /*!
     * Given temperature T in K, this method updates the values of
     * the non-dimensional heat capacity at constant pressure,
     * enthalpy, and entropy, at the reference pressure, Pref
     * of each of the standard states.
     *
     * @param t       Temperature (Kelvin)
     * @param cp_R    Vector of Dimensionless heat capacities.
     *                (length m_kk).
     * @param h_RT    Vector of Dimensionless enthalpies.
     *                (length m_kk).
     * @param s_R     Vector of Dimensionless entropies.
     *                (length m_kk).
     */
    virtual void update(doublereal t, doublereal* cp_R, 
			doublereal* h_RT, doublereal* s_R) const {
      int i;

      // load functions of temperature into m_t vector
      m_t[0] = t;
      m_t[1] = t*t;
      m_t[2] = m_t[1]*t;
      m_t[3] = m_t[2]*t;
      m_t[4] = 1.0/t;
      m_t[5] = log(t);

      // iterate over the groups
      vector<NasaPoly1>::const_iterator _begin, _end;
      for (i = 0; i != m_ngroups; i++) {
	if (t > m_tmid[i]) {
	  _begin  = m_high[i].begin();
	  _end    = m_high[i].end();
	}
	else {
	  _begin  = m_low[i].begin();
	  _end    = m_low[i].end();
	}
	for (; _begin != _end; ++_begin) 
	  _begin->updateProperties(&m_t[0], cp_R, h_RT, s_R);
      }
    }
                
    //! Minimum temperature.
    /*!
     * If no argument is supplied, this
     * method returns the minimum temperature for which \e all
     * parameterizations are valid. If an integer index k is
     * supplied, then the value returned is the minimum
     * temperature for species k in the phase.
     *
     * @param k    Species index
     */ 
    virtual doublereal minTemp(size_t k=npos) const {
      if (k == npos)
	return m_tlow_max;
      else
	return m_tlow[k];
    }

    //! Maximum temperature.
    /*!
     * If no argument is supplied, this
     * method returns the maximum temperature for which \e all
     * parameterizations are valid. If an integer index k is
     * supplied, then the value returned is the maximum
     * temperature for parameterization k.
     *
     * @param k Species index
     */
    virtual doublereal maxTemp(size_t k=npos) const {
      if (k == npos)
	return m_thigh_min;
      else
	return m_thigh[k];
    }
   
    //! The reference-state pressure for species k.
    /*!
     *
     * returns the reference state pressure in Pascals for
     * species k. If k is left out of the argument list,
     * it returns the reference state pressure for the first
     * species.
     * Note that some SpeciesThermo implementations, such
     * as those for ideal gases, require that all species
     * in the same phase have the same reference state pressures.
     *
     * @param k Species index
     */
    virtual doublereal refPressure(size_t k=npos) const {
      return m_p0;
    }

    //! This utility function reports the type of parameterization
    //! used for the species with index number index.
    /*!
     *
     * @param index  Species index
     */
    virtual int reportType(size_t index) const { return NASA; }

    /*!
     * This utility function reports back the type of 
     * parameterization and all of the parameters for the 
     * species, index.
     *
     * @param index     Species index
     * @param type      Integer type of the standard type
     * @param c         Vector of coefficients used to set the
     *                  parameters for the standard state.
     * For the NASA object, there are 15 coefficients.
     * @param minTemp   output - Minimum temperature
     * @param maxTemp   output - Maximum temperature
     * @param refPressure output - reference pressure (Pa).
     */
    virtual void reportParams(size_t index, int &type,
			      doublereal * const c, 
			      doublereal &minTemp, 
			      doublereal &maxTemp, 
			      doublereal &refPressure) const {
      type = reportType(index);
      if (type == NASA) {
	size_t grp = m_group_map[index];
	size_t pos = m_posInGroup_map[index];
	const vector<NasaPoly1> &mlg = m_low[grp-1];
	const vector<NasaPoly1> &mhg = m_high[grp-1];
	const NasaPoly1 *lowPoly  = &(mlg[pos]);
	const NasaPoly1 *highPoly = &(mhg[pos]);
	int itype = NASA;
	doublereal tmid = lowPoly->maxTemp();
	c[0] = tmid;
	size_t n;
	double ttemp;
	lowPoly->reportParameters(n, itype, minTemp, ttemp, refPressure,
				  c + 1);
	if (n != index) {
	  throw CanteraError("  ", "confused");
	}
	if (itype != NASA1) {
	  throw CanteraError("  ", "confused");
	}
	highPoly->reportParameters(n, itype, ttemp, maxTemp, refPressure,
				   c + 8);
	if (n != index) {
	  throw CanteraError("  ", "confused");
	}
	if (itype != NASA1) {
	  throw CanteraError("  ", "confused");
	}
      } else {
	throw CanteraError(" ", "confused");
      }
    }

    //! Modify parameters for the standard state
    /*!  
     * This utility function modifies the array of coefficients.
     * The array is the same as that returned by reportParams, so
     * a call can first be made to reportParams to populate the
     * array, and then modifyParams can be called to alter
     * selected values.  For the NASA object, there are 15
     * coefficients.

     * @param index Species index
     * @param c     Vector of coefficients used to set the
     *              parameters for the standard state.
     */
    virtual void modifyParams(size_t index, doublereal *c) {
      int type = reportType(index);
      if (type == NASA) {
	size_t grp = m_group_map[index];
	size_t pos = m_posInGroup_map[index];
	vector<NasaPoly1> &mlg = m_low[grp-1];
	vector<NasaPoly1> &mhg = m_high[grp-1];
	NasaPoly1 *lowPoly  = &(mlg[pos]);
	NasaPoly1 *highPoly = &(mhg[pos]);
	doublereal tmid = lowPoly->maxTemp();
	if (c[0] != tmid) {
	  throw CanteraError(" ", "Tmid cannot be changed");
	}
	lowPoly->modifyParameters(c + 1);
	highPoly->modifyParameters(c + 8);
	checkContinuity(m_name[index], c[0], c + 1, c + 8);
      } else {
	throw CanteraError(" ", "confused");
      }
    }

#ifdef H298MODIFY_CAPABILITY 
    virtual doublereal reportOneHf298(const int k) const {

      int grp = m_group_map[k];
      int pos = m_posInGroup_map[k];
      const vector<NasaPoly1> &mlg = m_low[grp-1];
      const NasaPoly1 *nlow = &(mlg[pos]);
      doublereal tmid = nlow->maxTemp();
      double h;
      if (298.15 <= tmid) {
	h = nlow->reportHf298(0);
      } else {
	const vector<NasaPoly1> &mhg = m_high[grp-1];
	const NasaPoly1 *nhigh = &(mhg[pos]);
	h = nhigh->reportHf298(0);
      }
      return h;
    }
 
    virtual void modifyOneHf298(const int k, const doublereal Hf298New) {
      int grp = m_group_map[k];
      int pos = m_posInGroup_map[k];
      vector<NasaPoly1> &mlg = m_low[grp-1];
      NasaPoly1 *nlow = &(mlg[pos]);
      vector<NasaPoly1> &mhg = m_high[grp-1];
      NasaPoly1 *nhigh = &(mhg[pos]);
      doublereal tmid = nlow->maxTemp();

      double hnow = reportOneHf298(k);
      double delH =  Hf298New - hnow;
      if (298.15 <= tmid) {
        nlow->modifyOneHf298(k, Hf298New);
	double h = nhigh->reportHf298(0);
	double hnew = h + delH;
	nhigh->modifyOneHf298(k, hnew);
      } else {
	nhigh->modifyOneHf298(k, Hf298New);
	double h = nlow->reportHf298(0);
	double hnew = h + delH;
	nlow->modifyOneHf298(k, hnew);
      }
 
    }
#endif

  protected:
    //! Vector of vector of NasaPoly1's for the high temp region.
    /*!
     * This is the high temp region representation.
     * The first Length is equal to the number of groups.
     * The second vector is equal to the number of species
     * in that particular group.
     */
    vector<vector<NasaPoly1> >         m_high;
 
    //! Vector of vector of NasaPoly1's for the low temp region.
    /*!
     * This is the low temp region representation.
     * The first Length is equal to the number of groups.
     * The second vector is equal to the number of species
     * in that particular group.
     */
    vector<vector<NasaPoly1> >         m_low;

    //! Map between the midpoint temperature, as an int, to the group number
    /*!
     * Length is equal to the number of groups. Only used in the setup.
     */
    map<int, int>                      m_index;

    //! Vector of log temperature limits
    /*!
     * Length is equal to the number of groups.
     */
    vector_fp                          m_tmid;

    //! Maximum value of the low temperature limit 
    doublereal                         m_tlow_max;

    //! Minimum value of the high temperature limit
    doublereal                         m_thigh_min;

    //! Vector of low temperature limits (species index)
    /*!
     * Length is equal to number of species
     */
    vector_fp                          m_tlow;

    //! Vector of low temperature limits (species index)
    /*!
     * Length is equal to number of species
     */
    vector_fp                          m_thigh;

    //! Reference pressure (Pa)
    /*!
     * all species must have the same reference pressure.
     */
    doublereal                         m_p0;

    //! number of groups
    int                                m_ngroups;

    //! Vector of temperature polynomials
    mutable vector_fp                  m_t;

    /*!
     * This map takes as its index, the species index in the phase.
     * It returns the group index, where the temperature polynomials
     * for that species are stored. group indecises start at 1,
     * so a decrement is always performed to access vectors.
     */
    mutable map<size_t, size_t> m_group_map;

    /*!
     * This map takes as its index, the species index in the phase.
     * It returns the position index within the group, where the 
     * temperature polynomials for that species are storred.
     */
    mutable map<size_t, size_t> m_posInGroup_map;

    //! Species name as a function of the species index
    mutable map<size_t, string> m_name;

  private:

    //! see SpeciesThermoFactory.cpp for the definition
    /*!
     * @param name string name of species
     * @param tmid  Mid temperature, between the two temperature regions
     * @param clow  coefficients for lower temperature region
     * @param chigh coefficients for higher temperature region
     */
    void checkContinuity(std::string name, double tmid, const doublereal* clow,
			 doublereal* chigh);

    //! for internal use by checkContinuity
    /*!
     * @param t temperature 
     * @param c coefficient array
     */
    doublereal enthalpy_RT(double t, const doublereal* c) {
      return c[0] + 0.5*c[1]*t + OneThird*c[2]*t*t 
	+ 0.25*c[3]*t*t*t + 0.2*c[4]*t*t*t*t
	+ c[5]/t;
    }

    //! for internal use by checkContinuity
    /*!
     * @param t temperature 
     * @param c coefficient array
     */
    doublereal entropy_R(double t, const doublereal* c) {
      return c[0]*log(t) + c[1]*t + 0.5*c[2]*t*t 
	+ OneThird*c[3]*t*t*t + 0.25*c[4]*t*t*t*t
	+ c[6];
    }



  };

}

#endif

