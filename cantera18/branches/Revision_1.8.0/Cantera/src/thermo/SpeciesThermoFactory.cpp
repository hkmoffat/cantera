/**
 *  @file SpeciesThermoFactory.cpp
 *    Definitions for factory to build instances of classes that manage the
 *    standard-state thermodynamic properties of a set of species 
 *    (see \ref spthermo and class \link Cantera::SpeciesThermoFactory SpeciesThermoFactory\endlink);
 */
/* 
 * $Revision$
 * $Date$
 */

// Copyright 2001  California Institute of Technology

#ifdef WIN32
#pragma warning(disable:4786)
#endif


#include "SpeciesThermoFactory.h"
using namespace std;

#include "SpeciesThermo.h"
#include "NasaThermo.h"
#include "ShomateThermo.h"
#include "SimpleThermo.h"
#include "GeneralSpeciesThermo.h"
#include "Mu0Poly.h"
#include "Nasa9PolyMultiTempRegion.h"
#include "Nasa9Poly1.h"

#ifdef WITH_ADSORBATE
#include "AdsorbateThermo.h"
#endif

#include "SpeciesThermoMgr.h"
#include "speciesThermoTypes.h"
#include "VPSSMgr.h"
#include "VPStandardStateTP.h"

#include "xml.h"
#include "ctml.h"

#include <cmath>

using namespace ctml;


namespace Cantera {

  SpeciesThermoFactory* SpeciesThermoFactory::s_factory = 0;

#if defined(THREAD_SAFE_CANTERA)
  boost::mutex SpeciesThermoFactory::species_thermo_mutex ;
#endif
 

  
  //! Examine the types of species thermo parameterizations,
  //! and return a flag indicating the type of reference state thermo manager
  //! that will be needed in order to evaluate them all.
  /*!
   * 
   *  @param spDataNodeList, This vector contains a list
   *                         of species XML nodes that will be in the phase
   * 
   * @todo Make sure that spDadta_node is species Data XML node by checking its name is speciesData
   */
  static void getSpeciesThermoTypes(std::vector<XML_Node *> & spDataNodeList, 
				    int& has_nasa, int& has_shomate, int& has_simple,
				    int &has_other) {
    size_t ns = spDataNodeList.size();
    for (size_t n = 0; n < ns; n++) {
      XML_Node* spNode = spDataNodeList[n];
      if (spNode->hasChild("standardState")) {
	const XML_Node& ss = spNode->child("standardState");
	string mname = ss["model"];
	if (mname == "water" || mname == "waterIAPWS") {
	  has_other = 1;
	  continue;
	}
      }
      if (spNode->hasChild("thermo")) {
	const XML_Node& th = spNode->child("thermo");
	if (th.hasChild("NASA")) {
	  has_nasa = 1;
	} else if (th.hasChild("Shomate")) {
	  has_shomate = 1;
	} else if (th.hasChild("MinEQ3")) {
	  has_shomate = 1;
	} else if (th.hasChild("const_cp")) {
	  has_simple = 1;
	} else if (th.hasChild("poly")) {
	  if (th.child("poly")["order"] == "1") has_simple = 1;
	  else throw CanteraError("newSpeciesThermo",
				  "poly with order > 1 not yet supported");
	}
	else if (th.hasChild("Mu0")) {
	  has_other = 1;
	} else if (th.hasChild("NASA9")) {
	  has_other = 1;
	} else if (th.hasChild("NASA9MULTITEMP")) {
	  has_other = 1;
	} else if (th.hasChild("adsorbate")) {
	  has_other = 1;
	} else {
	  has_other = 1;
	  //throw UnknownSpeciesThermoModel("getSpeciesThermoTypes:",
	  //	          	            spNode->attrib("name"), "missing");
	}
      } else {
	throw CanteraError("getSpeciesThermoTypes:",
			   spNode->attrib("name") + " is missing the thermo XML node");
      }
    }
  }

  //! Static method to return an instance of this class
  /*!
   * This class is implemented as a singleton -- one in which
   * only one instance is needed.  The recommended way to access
   * the factory is to call this static method, which
   * instantiates the class if it is the first call, but
   * otherwise simply returns the pointer to the existing
   * instance.
   */
  SpeciesThermoFactory* SpeciesThermoFactory::factory() {
#if defined(THREAD_SAFE_CANTERA)
     boost::mutex::scoped_lock lock(species_thermo_mutex);
#endif
     if (!s_factory) s_factory = new SpeciesThermoFactory;
     return s_factory;
  }

  // Delete static instance of this class
  /*
   * If it is necessary to explicitly delete the factory before
   * the process terminates (for example, when checking for
   * memory leaks) then this method can be called to delete it.
   */
  void SpeciesThermoFactory::deleteFactory() {
#if defined(THREAD_SAFE_CANTERA)
    boost::mutex::scoped_lock lock(species_thermo_mutex);
#endif
    if (s_factory) {
      delete s_factory;
      s_factory = 0;
    }
  }

  // Destructor
  /*
   * Doesn't do anything. We do not delete statically
   * created single instance of this class here, because it would
   * create an infinite loop if destructor is called for that
   * single instance.
   */
  SpeciesThermoFactory::~SpeciesThermoFactory() {
  }

  /*
   * Return a species thermo manager to handle the parameterizations
   * specified in a CTML phase specification.
   */
  SpeciesThermo* SpeciesThermoFactory::newSpeciesThermo(std::vector<XML_Node*> & spDataNodeList) const {
    int inasa = 0, ishomate = 0, isimple = 0, iother = 0;
    try {
      getSpeciesThermoTypes(spDataNodeList, inasa, ishomate, isimple, iother);
    } catch (UnknownSpeciesThermoModel) {
      iother = 1;
      popError();
    }
    if (iother) {
      //writelog("returning new GeneralSpeciesThermo");
      return new GeneralSpeciesThermo();
    }
    return newSpeciesThermo(NASA*inasa
			    + SHOMATE*ishomate + SIMPLE*isimple);
  }


  /*
   * @todo is this used? 
   */
  SpeciesThermo* SpeciesThermoFactory::
  newSpeciesThermoOpt(std::vector<XML_Node*> & spDataNodeList) const {
    int inasa = 0, ishomate = 0, isimple = 0, iother = 0;
    try {
      getSpeciesThermoTypes(spDataNodeList, inasa, ishomate, isimple, iother);
    } catch (UnknownSpeciesThermoModel) {
      iother = 1;
      popError();
    }
    
    if (iother) {
      return new GeneralSpeciesThermo();
    }
    return newSpeciesThermo(NASA*inasa
			    + SHOMATE*ishomate + SIMPLE*isimple);
  }

  SpeciesThermo* SpeciesThermoFactory::newSpeciesThermo(int type) const {
    switch (type) {
    case NASA:
      return new NasaThermo;
    case SHOMATE:
      return new ShomateThermo;
    case SIMPLE:
      return new SimpleThermo;
    case NASA + SHOMATE:
      return new SpeciesThermoDuo<NasaThermo, ShomateThermo>;
    case NASA + SIMPLE:
      return new SpeciesThermoDuo<NasaThermo, SimpleThermo>;
    case SHOMATE + SIMPLE:
      return new SpeciesThermoDuo<ShomateThermo, SimpleThermo>;
    default:
      throw UnknownSpeciesThermo("SpeciesThermoFactory::newSpeciesThermo",
				 type);
      return 0; 
    }
  }

  SpeciesThermo* SpeciesThermoFactory::newSpeciesThermoManager(std::string &stype) const {
    std::string ltype = lowercase(stype);
    if (ltype == "nasa") {
      return new NasaThermo;
    } else if (ltype == "shomate") {
      return new ShomateThermo;
    } else if (ltype ==  "simple" || ltype == "constant_cp") {
      return new SimpleThermo;
    } else if (ltype ==  "nasa_shomate_duo") {
      return new SpeciesThermoDuo<NasaThermo, ShomateThermo>;
    } else if (ltype ==  "nasa_simple_duo") {
      return new SpeciesThermoDuo<NasaThermo, SimpleThermo>;
    } else if (ltype ==  "shomate_simple_duo") {
      return new SpeciesThermoDuo<ShomateThermo, SimpleThermo>;
    } else if (ltype ==   "general") {
      return new GeneralSpeciesThermo();
    } else if (ltype ==  "") {
      return (SpeciesThermo*) 0;
    } else {
      throw UnknownSpeciesThermo("SpeciesThermoFactory::newSpeciesThermoManager",
				 stype);
    }
    return (SpeciesThermo*) 0;
  }

  /*
   * Check the continuity of properties at the midpoint
   * temperature.
   */
  void NasaThermo::checkContinuity(std::string name, double tmid, const doublereal* clow,
				   doublereal* chigh) {
    // heat capacity
    doublereal cplow = poly4(tmid, clow);
    doublereal cphigh = poly4(tmid, chigh);
    doublereal delta = cplow - cphigh;
    if (fabs(delta/(fabs(cplow)+1.0E-4)) > 0.001) {
      writelog("\n\n**** WARNING ****\nFor species "+name+
	       ", discontinuity in cp/R detected at Tmid = "
	       +fp2str(tmid)+"\n");
      writelog("\tValue computed using low-temperature polynomial:  "
	       +fp2str(cplow)+".\n");
      writelog("\tValue computed using high-temperature polynomial: "
	       +fp2str(cphigh)+".\n");
    }

    // enthalpy
    doublereal hrtlow = enthalpy_RT(tmid, clow);
    doublereal hrthigh = enthalpy_RT(tmid, chigh);
    delta = hrtlow - hrthigh;
    if (fabs(delta/(fabs(hrtlow)+cplow*tmid)) > 0.001) {
      writelog("\n\n**** WARNING ****\nFor species "+name+
	       ", discontinuity in h/RT detected at Tmid = "
	       +fp2str(tmid)+"\n");
      writelog("\tValue computed using low-temperature polynomial:  "
	       +fp2str(hrtlow)+".\n");
      writelog("\tValue computed using high-temperature polynomial: "
	       +fp2str(hrthigh)+".\n");
    }

    // entropy
    doublereal srlow = entropy_R(tmid, clow);
    doublereal srhigh = entropy_R(tmid, chigh);
    delta = srlow - srhigh;
    if (fabs(delta/(fabs(srlow)+cplow)) > 0.001) {
      writelog("\n\n**** WARNING ****\nFor species "+name+
	       ", discontinuity in s/R detected at Tmid = "
	       +fp2str(tmid)+"\n");
      writelog("\tValue computed using low-temperature polynomial:  "
	       +fp2str(srlow)+".\n");
      writelog("\tValue computed using high-temperature polynomial: "
	       +fp2str(srhigh)+".\n");
    }
  }


  /** 
   * Install a NASA polynomial thermodynamic property
   * parameterization for species k into a SpeciesThermo instance.
   * This is called by method installThermoForSpecies if a NASA
   * block is found in the XML input.
   */
  static void installNasaThermoFromXML(std::string speciesName,
				       SpeciesThermo& sp, int k, 
				       const XML_Node* f0ptr, const XML_Node* f1ptr) {
    doublereal tmin0, tmax0, tmin1, tmax1, tmin, tmid, tmax;

    const XML_Node& f0 = *f0ptr;

    // default to a single temperature range
    bool dualRange = false;

    // but if f1ptr is suppled, then it is a two-range
    // parameterization
    if (f1ptr) {dualRange = true;}

    tmin0 = fpValue(f0["Tmin"]);
    tmax0 = fpValue(f0["Tmax"]);

    doublereal p0 = OneAtm;
    if (f0.hasAttrib("P0")) {
      p0 = fpValue(f0["P0"]);
    }
    if (f0.hasAttrib("Pref")) {
      p0 = fpValue(f0["Pref"]);
    }
    p0 = OneAtm;

    tmin1 = tmax0;
    tmax1 = tmin1 + 0.0001;
    if (dualRange) {
      tmin1 = fpValue((*f1ptr)["Tmin"]);
      tmax1 = fpValue((*f1ptr)["Tmax"]);
    }

    vector_fp c0, c1;
    if (fabs(tmax0 - tmin1) < 0.01) {
      // f0 has the lower T data, and f1 the higher T data
      tmin = tmin0;
      tmid = tmax0;
      tmax = tmax1;
      getFloatArray(f0.child("floatArray"), c0, false);
      if (dualRange)
	getFloatArray(f1ptr->child("floatArray"), c1, false);
      else {
	// if there is no higher range data, then copy c0 to c1.
	c1.resize(7,0.0);
	copy(c0.begin(), c0.end(), c1.begin());
      }
    }
    else if (fabs(tmax1 - tmin0) < 0.01) {
      // f1 has the lower T data, and f0 the higher T data
      tmin = tmin1;
      tmid = tmax1;
      tmax = tmax0;
      getFloatArray(f1ptr->child("floatArray"), c0, false);
      getFloatArray(f0.child("floatArray"), c1, false);
    }
    else {
      throw CanteraError("installNasaThermo",
			 "non-continuous temperature ranges.");
    }

    // The NasaThermo species property manager expects the
    // coefficients in a different order, so rearrange them.
    array_fp c(15);
    c[0] = tmid;
   
    c[1] = c0[5];
    c[2] = c0[6];
    copy(c0.begin(), c0.begin()+5, c.begin() + 3);
    c[8] = c1[5];
    c[9] = c1[6];
    copy(c1.begin(), c1.begin()+5, c.begin() + 10);
    sp.install(speciesName, k, NASA, &c[0], tmin, tmax, p0);
  }

#ifdef INCL_NASA96

  /** 
   * Install a NASA96 polynomial thermodynamic property
   * parameterization for species k into a SpeciesThermo instance.
   */
  static void installNasa96ThermoFromXML(std::string speciesName,
					 SpeciesThermo& sp, int k, 
					 const XML_Node* f0ptr, const XML_Node* f1ptr) {
    doublereal tmin0, tmax0, tmin1, tmax1, tmin, tmid, tmax;

    const XML_Node& f0 = *f0ptr;
    bool dualRange = false;
    if (f1ptr) {dualRange = true;}
    tmin0 = fpValue(f0["Tmin"]);
    tmax0 = fpValue(f0["Tmax"]);
    tmin1 = tmax0;
    tmax1 = tmin1 + 0.0001;
    if (dualRange) {
      tmin1 = fpValue((*f1ptr)["Tmin"]);
      tmax1 = fpValue((*f1ptr)["Tmax"]);
    }


    doublereal p0 = OneAtm;
    if (f0.hasAttrib("P0")) {
      p0 = fpValue(f0["P0"]);
    }
    if (f0.hasAttrib("Pref")) {
      p0 = fpValue(f0["Pref"]);
    }

    vector_fp c0, c1;
    if (fabs(tmax0 - tmin1) < 0.01) {
      tmin = tmin0;
      tmid = tmax0;
      tmax = tmax1;
      getFloatArray(f0.child("floatArray"), c0, false);
      if (dualRange)
	getFloatArray(f1ptr->child("floatArray"), c1, false);
      else {
	c1.resize(7,0.0);
	copy(c0.begin(), c0.end(), c1.begin());
      }
    }
    else if (fabs(tmax1 - tmin0) < 0.01) {
      tmin = tmin1;
      tmid = tmax1;
      tmax = tmax0;
      getFloatArray(f1ptr->child("floatArray"), c0, false);
      getFloatArray(f0.child("floatArray"), c1, false);
    }
    else {
      throw CanteraError("installNasaThermo",
			 "non-continuous temperature ranges.");
    }
    array_fp c(15);
    c[0] = tmid;
    c[1] = c0[5];
    c[2] = c0[6];
    copy(c0.begin(), c0.begin()+5, c.begin() + 3);
    c[8] = c1[5];
    c[9] = c1[6];
    copy(c1.begin(), c1.begin()+5, c.begin() + 10);
    sp.install(speciesName, k, NASA, &c[0], tmin, tmax, p0);
  }

#endif

  static doublereal LookupGe(const std::string& elemName, ThermoPhase *th_ptr) {
#ifdef OLDWAY
    int num = sizeof(geDataTable) / sizeof(struct GeData);
    string s3 = elemName.substr(0,3);
    for (int i = 0; i < num; i++) {
      //if (!std::strncmp(elemName.c_str(), aWTable[i].name, 3)) {
      if (s3 == geDataTable[i].name) {
        return (geDataTable[i].GeValue);
      }
    }
    throw CanteraError("LookupGe", "element " + s + " not found");
    return -1.0;
#else
    int iE = th_ptr->elementIndex(elemName);
    if (iE < 0) {
      throw CanteraError("PDSS_HKFT::LookupGe", "element " + elemName + " not found");
    }
    doublereal geValue = th_ptr->entropyElement298(iE);
    if (geValue == ENTROPY298_UNKNOWN) {
      throw CanteraError("PDSS_HKFT::LookupGe",
                         "element " + elemName + " doesn not have a supplied entropy298");
    }
    geValue *= (-298.15);
    return geValue;
#endif
  }

 static doublereal convertDGFormation(int k, ThermoPhase *th_ptr) {
    /*
     * Ok let's get the element compositions and conversion factors.
     */
    int ne = th_ptr->nElements();
    doublereal na;
    doublereal ge;
    string ename;

    doublereal totalSum = 0.0;
    for (int m = 0; m < ne; m++) {
      na = th_ptr->nAtoms(k, m);
      if (na > 0.0) {
	ename = th_ptr->elementName(m);
	ge = LookupGe(ename, th_ptr);
	totalSum += na * ge;
      }
    }
    return totalSum;
  }



  static void installMinEQ3asShomateThermoFromXML(std::string speciesName, 
						  ThermoPhase *th_ptr,
						  SpeciesThermo& sp, int k, 
						  const XML_Node* MinEQ3node) {

    array_fp coef(15), c0(7, 0.0);
    std::string astring = (*MinEQ3node)["Tmin"];
    doublereal tmin0 = strSItoDbl(astring);
    astring = (*MinEQ3node)["Tmax"];
    doublereal tmax0 = strSItoDbl(astring);
    astring = (*MinEQ3node)["Pref"];
    doublereal p0 = strSItoDbl(astring);
 
    doublereal deltaG_formation_pr_tr =
      getFloatDefaultUnits(*MinEQ3node, "DG0_f_Pr_Tr", "cal/gmol", "actEnergy");
    doublereal deltaH_formation_pr_tr =
      getFloatDefaultUnits(*MinEQ3node, "DH0_f_Pr_Tr", "cal/gmol", "actEnergy");
    doublereal Entrop_pr_tr = getFloatDefaultUnits(*MinEQ3node, "S0_Pr_Tr", "cal/gmol/K");
    doublereal a = getFloatDefaultUnits(*MinEQ3node, "a", "cal/gmol/K");
    doublereal b = getFloatDefaultUnits(*MinEQ3node, "b", "cal/gmol/K2");
    doublereal c = getFloatDefaultUnits(*MinEQ3node, "c", "cal-K/gmol");
    doublereal dg = deltaG_formation_pr_tr * 4.184 * 1.0E3;
    doublereal fac =  convertDGFormation(k, th_ptr);
    doublereal Mu0_tr_pr = fac + dg;
    doublereal e = Entrop_pr_tr * 1.0E3 * 4.184;
    doublereal Hcalc = Mu0_tr_pr + 298.15 * e;
    doublereal DHjmol = deltaH_formation_pr_tr * 1.0E3 * 4.184;

    // If the discrepency is greater than 100 cal gmol-1, print
    // an error and exit.
    if (fabs(Hcalc -DHjmol) > 10.* 1.0E6 * 4.184) {
      throw CanteraError("installMinEQ3asShomateThermoFromXML()",
                         "DHjmol is not consistent with G and S" +
                         fp2str(Hcalc) + " vs " + fp2str(DHjmol));
    }

    /*
     * Now calculate the shomate polynomials
     *
     * Cp first
     * 
     *  Shomate: (Joules / gmol / K)
     *    Cp = As + Bs * t + Cs * t*t + Ds * t*t*t + Es / (t*t)
     *     where
     *          t = temperature(Kelvin) / 1000
     */
    double As = a * 4.184;
    double Bs = b * 4.184 * 1000.;
    double Cs = 0.0;
    double Ds = 0.0;
    double Es = c * 4.184 / (1.0E6);
    
    double t = 298.15 / 1000.;
    double H298smFs = As * t + Bs * t * t / 2.0 - Es / t; 
    
    double HcalcS = Hcalc / 1.0E6;
    double Fs = HcalcS - H298smFs;

    double S298smGs = As * log(t) + Bs * t - Es/(2.0*t*t);
    double ScalcS = e / 1.0E3;
    double Gs = ScalcS - S298smGs;

    c0[0] = As;
    c0[1] = Bs;
    c0[2] = Cs;
    c0[3] = Ds;
    c0[4] = Es;
    c0[5] = Fs;
    c0[6] = Gs;

    coef[0] = tmax0 - 0.001;
    copy(c0.begin(), c0.begin()+7, coef.begin() + 1);
    copy(c0.begin(), c0.begin()+7, coef.begin() + 8);
    sp.install(speciesName, k, SHOMATE, &coef[0], tmin0, tmax0, p0);
  }


  /** 
   * Install a Shomate polynomial thermodynamic property
   * parameterization for species k.
   */
  static void installShomateThermoFromXML(std::string speciesName, 
					  SpeciesThermo& sp, int k, 
					  const XML_Node* f0ptr, const XML_Node* f1ptr) {
    doublereal tmin0, tmax0, tmin1, tmax1, tmin, tmid, tmax;

    const XML_Node& f0 = *f0ptr;
    bool dualRange = false;
    if (f1ptr) {dualRange = true;}
    tmin0 = fpValue(f0["Tmin"]);
    tmax0 = fpValue(f0["Tmax"]);
    tmin1 = tmax0;
    tmax1 = tmin1 + 0.0001;
    if (dualRange) {
      tmin1 = fpValue((*f1ptr)["Tmin"]);
      tmax1 = fpValue((*f1ptr)["Tmax"]);
    }

    vector_fp c0, c1;
    if (fabs(tmax0 - tmin1) < 0.01) {
      tmin = tmin0;
      tmid = tmax0;
      tmax = tmax1;
      getFloatArray(f0.child("floatArray"), c0, false);
      if (dualRange)
	getFloatArray(f1ptr->child("floatArray"), c1, false);
      else {
	c1.resize(7,0.0);
	copy(c0.begin(), c0.begin()+7, c1.begin());
      }
    }
    else if (fabs(tmax1 - tmin0) < 0.01) {
      tmin = tmin1;
      tmid = tmax1;
      tmax = tmax0;
      getFloatArray(f1ptr->child("floatArray"), c0, false);
      getFloatArray(f0.child("floatArray"), c1, false);
    }
    else {
      throw CanteraError("installShomateThermoFromXML",
			 "non-continuous temperature ranges.");
    }
    array_fp c(15);
    c[0] = tmid;
    doublereal p0 = OneAtm;
    copy(c0.begin(), c0.begin()+7, c.begin() + 1);
    copy(c1.begin(), c1.begin()+7, c.begin() + 8);
    sp.install(speciesName, k, SHOMATE, &c[0], tmin, tmax, p0);
  }



  /** 
   * Install a constant-cp thermodynamic property
   * parameterization for species k.
   */
  static void installSimpleThermoFromXML(std::string speciesName, 
					 SpeciesThermo& sp, int k, 
					 const XML_Node& f) {
    doublereal tmin, tmax;
    tmin = fpValue(f["Tmin"]);
    tmax = fpValue(f["Tmax"]);
    if (tmax == 0.0) tmax = 1.0e30;

    vector_fp c(4);
    c[0] = getFloat(f, "t0", "toSI");
    c[1] = getFloat(f, "h0", "toSI");
    c[2] = getFloat(f, "s0", "toSI");
    c[3] = getFloat(f, "cp0", "toSI");
    doublereal p0 = OneAtm;
    sp.install(speciesName, k, SIMPLE, &c[0], tmin, tmax, p0);
  }

  /** 
   * Install a NASA9 polynomial thermodynamic property
   * parameterization for species k into a SpeciesThermo instance.
   * This is called by method installThermoForSpecies if a NASA9
   * block is found in the XML input.
   */
  static void installNasa9ThermoFromXML(std::string speciesName,
					SpeciesThermo& sp, int k, 
					const std::vector<XML_Node*>& tp)
  { 				
    const XML_Node * fptr = tp[0];
    int nRegTmp = tp.size();
    int nRegions = 0;
    vector_fp cPoly;
    Nasa9Poly1 *np_ptr = 0; 
    std::vector<Nasa9Poly1 *> regionPtrs;
    doublereal tmin, tmax, pref = OneAtm;
    // Loop over all of the possible temperature regions
    for (int i = 0; i < nRegTmp; i++) {
      fptr = tp[i];
      if (fptr) {
	if (fptr->name() == "NASA9") {
	  if (fptr->hasChild("floatArray")) {

	    tmin = fpValue((*fptr)["Tmin"]);
	    tmax = fpValue((*fptr)["Tmax"]);
	    if ((*fptr).hasAttrib("P0")) {
	      pref = fpValue((*fptr)["P0"]);
	    }
	    if ((*fptr).hasAttrib("Pref")) {
	      pref = fpValue((*fptr)["Pref"]);
	    }

	    getFloatArray(fptr->child("floatArray"), cPoly, false);
	    if (cPoly.size() != 9) {
	      throw CanteraError("installNasa9ThermoFromXML",
				 "Expected 9 coeff polynomial");
	    }
	    np_ptr = new Nasa9Poly1(k, tmin, tmax, pref,
				    DATA_PTR(cPoly));
	    regionPtrs.push_back(np_ptr);
	    nRegions++;
	  } 
	}
      }
    }
    if (nRegions == 0) {
      throw UnknownSpeciesThermoModel("installThermoForSpecies", 
				      speciesName, "  " );
    } else if (nRegions == 1)  {
      sp.install_STIT(np_ptr);
    } else {
      Nasa9PolyMultiTempRegion*  npMulti_ptr = new  Nasa9PolyMultiTempRegion(regionPtrs);
      sp.install_STIT(npMulti_ptr);
    }
  }


  /** 
   * Install an Adsorbate thermodynamic property
   * parameterization for species k into a SpeciesThermo instance.
   * This is called by method installThermoForSpecies if a NASA9
   * block is found in the XML input.
   */
#ifdef WITH_ADSORBATE
  static void installAdsorbateThermoFromXML(std::string speciesName,
					    SpeciesThermo& sp, int k, 
					    const XML_Node& f) { 		
    vector_fp freqs;
    doublereal tmin, tmax, pref = OneAtm;
    int nfreq = 0;
    tmin = fpValue(f["Tmin"]);
    tmax = fpValue(f["Tmax"]);
    if (f.hasAttrib("P0")) {
      pref = fpValue(f["P0"]);
    }
    if (f.hasAttrib("Pref")) {
      pref = fpValue(f["Pref"]);
    }
    if (tmax == 0.0) tmax = 1.0e30;

    if (f.hasChild("floatArray")) {
      getFloatArray(f.child("floatArray"), freqs, false);
      nfreq = freqs.size(); 
    }
    for (int n = 0; n < nfreq; n++) {
      freqs[n] *= 3.0e10;
    }
    vector_fp coeffs(nfreq + 2);
    coeffs[0] = nfreq;
    coeffs[1] = getFloat(f, "binding_energy", "toSI");
    copy(freqs.begin(), freqs.end(), coeffs.begin() + 2);
    //posc = new Adsorbate(k, tmin, tmax, pref,  
    //    DATA_PTR(coeffs)); 
    (&sp)->install(speciesName, k, ADSORBATE, &coeffs[0], tmin, tmax, pref);
  }
#endif

  /**
   * Install a species thermodynamic property parameterization
   * for one species into a species thermo manager.
   * @param k species number
   * @param s XML node specifying species
   * @param spthermo species thermo manager
   * @param phaseNode_ptr Optional Pointer to the XML phase
   *                      information for the phase in which the species
   *                      resides
   */
  void SpeciesThermoFactory::
  installThermoForSpecies(int k, const XML_Node& s, ThermoPhase *th_ptr,
			  SpeciesThermo& spthermo,
			  const XML_Node *phaseNode_ptr) const {
    /*
     * Check to see that the species block has a thermo block
     * before processing. Throw an error if not there.
     */
    if (!(s.hasChild("thermo"))) {
      throw UnknownSpeciesThermoModel("installThermoForSpecies", 
				      s["name"], "<nonexistent>");
    }
    const XML_Node& thermo = s.child("thermo");
    const std::vector<XML_Node*>& tp = thermo.children();
    int nc = static_cast<int>(tp.size());
    string mname = thermo["model"];

    if (mname == "MineralEQ3") {
      const XML_Node* f = tp[0];
      if (f->name() != "MinEQ3") {
	throw CanteraError("SpeciesThermoFactory::installThermoForSpecies",
			   "confused: expedted MinEQ3");
      }
      installMinEQ3asShomateThermoFromXML(s["name"], th_ptr, spthermo, k, f);
    } else {
      if (nc == 1) {
	const XML_Node* f = tp[0];
	if (f->name() == "Shomate") {
	  installShomateThermoFromXML(s["name"], spthermo, k, f, 0);
	}
	else if (f->name() == "const_cp") {
	  installSimpleThermoFromXML(s["name"], spthermo, k, *f);
	}
	else if (f->name() == "NASA") {
	  installNasaThermoFromXML(s["name"], spthermo, k, f, 0);
	}
	else if (f->name() == "Mu0") {
	  installMu0ThermoFromXML(s["name"], spthermo, k, f);
	}
	else if (f->name() == "NASA9") {
	  installNasa9ThermoFromXML(s["name"], spthermo, k, tp);
	}
	// else if (f->name() == "HKFT") {
	//	installHKFTThermoFromXML(s["name"], spthermo, k, tp);
	//}
#ifdef WITH_ADSORBATE
	else if (f->name() == "adsorbate") {
	  installAdsorbateThermoFromXML(s["name"], spthermo, k, *f);
	}
#endif
	else {
	  throw UnknownSpeciesThermoModel("installThermoForSpecies", 
					  s["name"], f->name());
	}
      }
      else if (nc == 2) {
	const XML_Node* f0 = tp[0];
	const XML_Node* f1 = tp[1];
	if (f0->name() == "NASA" && f1->name() == "NASA") {
	  installNasaThermoFromXML(s["name"], spthermo, k, f0, f1);
	} 
	else if (f0->name() == "Shomate" && f1->name() == "Shomate") {
	  installShomateThermoFromXML(s["name"], spthermo, k, f0, f1);
	} 
	else if (f0->name() == "NASA9" && f1->name() == "NASA9") {
	  installNasa9ThermoFromXML(s["name"], spthermo, k, tp);
	} else {
	  throw UnknownSpeciesThermoModel("installThermoForSpecies", s["name"], 
					  f0->name() + " and "
					  + f1->name());
	}
      }
      else if (nc >= 2) {
	const XML_Node* f0 = tp[0];
	if (f0->name() == "NASA9") {
	  installNasa9ThermoFromXML(s["name"], spthermo, k, tp);
	} else {
	  throw UnknownSpeciesThermoModel("installThermoForSpecies", s["name"], 
					  "multiple");
	}
      } else {
	throw UnknownSpeciesThermoModel("installThermoForSpecies", s["name"], 
					"multiple");
      }
    }
  }


  // Install a species thermodynamic property parameterization
  // for the standard state for one species into a species thermo manager, VPSSMgr
  /*
   * This is a wrapper around the createInstallVPSS() function in the 
   * VPStandardStateTP object.
   *
   * This serves to install the species into vpss_ptr, create a PDSS file. We also
   * read the xml database to extract the constants for these steps.
   *
   * @param k             species number
   * @param speciesNode   Reference to the XML node specifying the species standard
   *                      state information
   * @param vp_ptr        variable pressure ThermoPhase object 
   * @param vpss_ptr      Pointer to the Manager for calculating variable pressure
   *                      substances.
   * @param spthermo_ptr  Species reference state thermo manager
   * @param phaseNode_ptr Optional Pointer to the XML phase
   *                      information for the phase in which the species
   *                      resides
   */
  void SpeciesThermoFactory::
  installVPThermoForSpecies(int k, const XML_Node& speciesNode, 
			    VPStandardStateTP *vp_ptr,
			    VPSSMgr *vpssmgr_ptr,
			    SpeciesThermo *spthermo_ptr,
			    const XML_Node *phaseNode_ptr) const {
    
    // Call the VPStandardStateTP object to install the pressure dependent species
    // standard state into the object.
    //
    // We don't need to pass spthermo_ptr down, because it's already installed
    // into vp_ptr.
    //
    // We don't need to pass vpssmgr_ptr down, because it's already installed
    // into vp_ptr.
    vp_ptr->createInstallPDSS(k, speciesNode,  phaseNode_ptr);
  }

  // Create a new species thermo manager instance, by specifying
  // the type and (optionally) a pointer to the factory to use to create it.
  /*
   * This utility program  will look through species nodes. It will discover what
   * each species needs for its species property managers. Then,
   * it will malloc and return the proper species property manager to use.
   *
   *  These functions allow using a different factory class that
   *  derives from SpeciesThermoFactory.
   *
   * @param type         Species thermo type.
   * @param f            Pointer to a SpeciesThermoFactory. optional parameter. 
   *                    Defautls to NULL.
   */
  SpeciesThermo* newSpeciesThermoMgr(int type, SpeciesThermoFactory* f) {
    if (f == 0) {
      f = SpeciesThermoFactory::factory();
    }
    SpeciesThermo* sptherm = f->newSpeciesThermo(type);
    return sptherm;
  }

  // Create a new species thermo manager instance, by specifying
  //the type and (optionally) a pointer to the factory to use to create it.
  /*
   * This utility program is a basic factory operation for spawning a
   * new species reference-state thermo mananger
   *
   *  These functions allows for using a different factory class that
   *  derives from SpeciesThermoFactory. However, no applications of this
   *  have been done yet.
   *
   * @param stype       String specifying the species thermo type
   * @param f           Pointer to a SpeciesThermoFactory. optional parameter. 
   *                    Defaults to NULL.
   */
  SpeciesThermo* newSpeciesThermoMgr(std::string &stype, 
				     SpeciesThermoFactory* f) {
    if (f == 0) {
      f = SpeciesThermoFactory::factory();
    }
    SpeciesThermo* sptherm = f->newSpeciesThermoManager(stype);
    return sptherm;
  }
  
  // Function to return SpeciesThermo manager
  /*
   * This utility program  will look through species nodes. It will discover what
   * each species needs for its species property managers. Then,
   * it will malloc and return the proper species property manager to use.
   *
   *  These functions allow using a different factory class that
   *  derives from SpeciesThermoFactory.
   *
   * @param spData_nodes Vector of XML_Nodes, each of which is a speciesData XML Node.
   *                     Each %speciesData node contains a list of XML species elements
   *                      e.g., \<speciesData id="Species_Data"\>
   * @param f            Pointer to a SpeciesThermoFactory. optional parameter. 
   *                    Defautls to NULL.
   * @param opt         Boolean defaults to false.
   */
  SpeciesThermo* newSpeciesThermoMgr(std::vector<XML_Node*> spData_nodes, 
				     SpeciesThermoFactory* f, bool opt) {
    if (f == 0) {
      f = SpeciesThermoFactory::factory();
    }
    SpeciesThermo* sptherm;
    if (opt) {
      sptherm = f->newSpeciesThermoOpt(spData_nodes);
    } else { 
      sptherm = f->newSpeciesThermo(spData_nodes);
    }
    return sptherm;
  }

}
