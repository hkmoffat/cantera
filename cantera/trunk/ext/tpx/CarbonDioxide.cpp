/* FILE: CarbonDioxide.cpp
 * DESCRIPTION: 
 *  representation of substance Carbon Dioxide 
 *  values and functions are from
 *  "Thermodynamic Properties in SI" bu W.C. Reynolds
 * AUTHOR: me@rebeccahhunt.com: GCEP, Stanford University
 *
 */

#include "CarbonDioxide.h"
#include <math.h>
#include <string.h>

namespace tpx {

/* 
 * Carbon Dioxide constants
 */
static const double Tmn = 216.54;   // [K] minimum temperature for which calculations are valid
static const double Tmx = 1500.0;   // [K] maximum temperature for which calculations are valid
static const double Tc=304.21;      // [K] critical temperature
static const double Roc=464.00;        // [kg/m^3] critical density
static const double To=216.54;        // [K] reference Temperature 
static const double R=188.918;        // [] gas constant for CO2 J/kg/K
static const double Gamma=5.0E-6;    // [??]
static const double u0=3.217405E5;    // [] internal energy at To
static const double s0=2.1396056E3;    // [] entropy at To
static const double Tp=250;            // [K] ??
//static const double Pc=7.38350E6;    // [Pa] critical pressure
static const double Pc=7.3817589E6;   // Adjusted to fit the actual functional form
static const double M=44.01;        // [kg/kmol] molar density

/*
 * array Acarbdi is used by the function named Pp
 */
static const double Acarbdi[]={
 2.2488558E-1,
-1.3717965E2,  
-1.4430214E4,
-2.9630491E6,   
-2.0606039E8,   
 4.5554393E-5,
 7.7042840E-2,   
 4.0602371E1,   
 4.0029509E-7,
-3.9436077E-4,   
 1.2115286E-10, 
 1.0783386E-7,
 4.3962336E-11, 
-3.6505545E4,   
 1.9490511E7,
-2.9186718E9,   
 2.4358627E-2,    
 -3.7546530E1,
 1.1898141E4
};
    

/*
 * array F is used by the function named Psat 
 */
static const double F[]={
    -6.5412610,
    -2.7914636E-1,
    -3.4716202,
    -3.4989637,
    -1.9770948E1,
     1.3922839E2,
    -2.7670389E2,
    -7.0510251E3
};


/*
 * array D is used by the function ldens
 */
static const double D[]={
    4.6400009E2,
    6.7938129E2,
    1.4776836E3,
   -3.1267676E3,
    3.6397656E3,
   -1.3437098E3
};


/*
 * array G is used by the function sp
 */
static const double G[]={
    8.726361E3,
    1.840040E2,
    1.914025,
   -1.667825E-3,
    7.305950E-7,
   -1.255290E-10,
    3.2174105E5,
    2.1396056E3

};

/* 
 * C returns a multiplier in each term of the sum 
 * in P-3, used in conjunction with C in the function Pp
 * j is used to represent which of the values in the summation to calculate
 * j=0 is the second additive in the formula in reynolds
 * j=1 is the third...
 * (this part does not include the multiplier rho^n)
 */
 double CarbonDioxide::C(int j,double Tinverse, double T2inverse, double T3inverse, double T4inverse) {
    switch(j) {
    case 0 : 
        return    Acarbdi[0]*T            +
                Acarbdi[1]                +
                Acarbdi[2] * Tinverse    +
                Acarbdi[3] * T2inverse    +
                Acarbdi[4] * T3inverse ;
    case 1 : 
        return    Acarbdi[5] *T            +
                Acarbdi[6]                +
                Acarbdi[7] * Tinverse ;
    case 2 :
        return Acarbdi[8]*T + Acarbdi[9];
    case 3 :
        return Acarbdi[10]*T  + Acarbdi[11];
    case 4 :
        return Acarbdi[12];
    case 5 :
        return    Acarbdi[13] *T2inverse    +
                Acarbdi[14] *T3inverse    +
                Acarbdi[15] *T4inverse;
    case 6 :
        return    Acarbdi[16] *T2inverse    +
                Acarbdi[17] *T3inverse    +
                Acarbdi[18] *T4inverse;
    default :
        return 0.0;
    }
}

 /* cprime
  * derivative of C(i)
  */
inline double CarbonDioxide::Cprime(int j, double T2inverse, double T3inverse, double T4inverse) {
    
    switch(j) {
    case 0 : 
        return    Acarbdi[0]     +                
              - Acarbdi[2] * T2inverse    +
            -2 * Acarbdi[3] * T3inverse    +
            -3 * Acarbdi[4] * T4inverse ;
    case 1 : 
        return    Acarbdi[5]     -                
                Acarbdi[7] * T2inverse;
    case 2 :
        return  Acarbdi[8] ;
    case 3 :
        return    Acarbdi[10] ;
    case 4 :
        return    0;
    case 5 :
        return    
            -2 *Acarbdi[13] *T3inverse    +
            -3 *Acarbdi[14] *T4inverse    +
            -4 *Acarbdi[15]* pow(T,-5);
    case 6 :
        return    
            -2 *Acarbdi[16] *T3inverse    +
            -3 *Acarbdi[17] *T4inverse    +
            -4 *Acarbdi[18] *pow(T,-5);
    default :
        return 0.0;
    }
}

/*
 * I = integral from o-rho { 1/(rho^2) * H(i, rho) d rho } 
 * ( see section 2 of Reynolds TPSI )
 */
inline double CarbonDioxide::I(int j, double ergho, double Gamma) {    
    switch (j) {
        
    case 0:
        return Rho;
    case 1: 
        return pow(Rho, 2)/2;
    case 2: 
        return pow(Rho, 3)/ 3;
    case 3:    
        return pow(Rho, 4)/ 4;
    case 4:    
        return pow(Rho, 5)/ 5;        
    case 5:    
        return (1 - ergho ) / double(2 * Gamma);
    case 6:        
        return ( 1 - ergho * double( Gamma * pow(Rho,2) + double(1) ) )/ double(2 * Gamma * Gamma);
    default:  
        return 0.0;
    }
}


/* H returns a multiplier in each term of the sum 
 * in P-3
 * this is used in conjunction with C in the function Pp
 * this represents the product rho^n
 * i=0 is the second additive in the formula in reynolds
 * i=1 is the third ...
 */
double CarbonDioxide::H(int i, double egrho) {        
    if (i < 5)
        return pow(Rho,i+2);
    else if (i == 5)
        return pow(Rho,3)*egrho;
    else if (i == 6)
        return pow(Rho,5)*egrho;
    else 
        return 0;
}

/* 
 * internal energy
 *  see Reynolds eqn (15) section 2
 *  u = (the integral from T to To of co(T)dT) +
 *         sum from i to N ([C(i) - T*Cprime(i)] + uo
 */
double CarbonDioxide::up() {
    
  double Tinverse = 1.0/T;   
  double T2inverse = pow(T, -2);
  double T3inverse = pow(T, -3);
  double T4inverse = pow(T, -4);
  double egrho = exp(-Gamma*Rho*Rho);

   double sum = 0.0;

   // Equation C-6 integrated   
   sum += G[0]*log(T/To);
   int i;
   for (i=1; i<=5; i++)
      sum += G[i]*(pow(T,i) - pow(To,i))/double(i);


   for (i=0; i<=6; i++) {
       sum += I(i,egrho, Gamma) *
           ( C(i, Tinverse, T2inverse, T3inverse, T4inverse) - T*Cprime(i,T2inverse, T3inverse, T4inverse) );   
   }
    
   sum += u0;
   return sum + m_energy_offset;

 }

/*
*  entropy
 *  see Reynolds eqn (16) section 2
*/

double CarbonDioxide::sp() {
    //double Tinverse = 1.0/T;   
    double T2inverse = pow(T, -2);
    double T3inverse = pow(T, -3);
    double T4inverse = pow(T, -4);
    double egrho = exp(-Gamma*Rho*Rho);
    
    double sum = 0.0;

    for (int i=2; i<=5; i++)
        sum += G[i]*(pow(T,i-1) - pow(To,i-1))/double(i-1);

    sum += G[1]*log(T/To);
    sum -= G[0]*(1.0/To - 1.0/T);
    

    for (int i=0; i<=6; i++) {
        sum -= Cprime(i,T2inverse, T3inverse, T4inverse)*I(i,egrho,Gamma);
    }

    sum += s0 - R*log(Rho);

    return sum + m_entropy_offset;
}
 

/* 
 * Equation P-3 in Reynolds
 * P - rho - T
 * returns P (pressure)
 */
double CarbonDioxide::Pp(){
  double Tinverse = pow(T,-1);   
  double T2inverse = pow(T, -2);
  double T3inverse = pow(T, -3);
  double T4inverse = pow(T, -4);
  double egrho = exp(-Gamma*Rho*Rho);
    
    double P = Rho*R*T;    

    // when i=0 we are on second sum of equation (where rho^2)
    for(int i=0; i<=6; i++) {        
        P += C(i,Tinverse, T2inverse, T3inverse, T4inverse)*H(i,egrho);    
    }
    return P;
}


/* 
 * Equation S-2 in Reynolds
 * Pressure at Saturation
 */
double CarbonDioxide::Psat(){

    double log, sum=0,P;
    if ((T < Tmn) || (T > Tc)) {
        std::cout << " error in Psat " << TempError << std::endl;
        set_Err(TempError); // Error("CarbonDioxide::Psat",TempError,T);
    }
    for (int i=1;i<=8;i++)
        sum += F[i-1] * pow((T/Tp -1),double(i-1)); 

    log = ((Tc/T)-1)*sum;
    P=exp(log)*Pc;

    //cout << "Psat is returning " << P << " at T " << T << " and Pc " << Pc << " and Tp " << Tp << endl;
    return P;
        
}

/*
 * Equation D2 in Reynolds
 * liquid density, of rho_f
 */
double CarbonDioxide::ldens() {    
    double xx=1-(T/Tc), sum=0;
    if ((T < Tmn) || (T > Tc)) {        
        std::cout << " error in ldens " << TempError << std::endl;
        set_Err(TempError);
    }
    for(int i=1;i<=6;i++)
        sum+=D[i-1]*pow(xx,double(i-1)/3.0);

    return sum;
}

/* 
 * the following functions allow users
 * to get the properties of CarbonDioxide
 * that are not dependent on the state
 */
double CarbonDioxide::Tcrit() {return Tc;}
double CarbonDioxide::Pcrit() {return Pc;}
double CarbonDioxide::Vcrit() {return 1.0/Roc;}
double CarbonDioxide::Tmin() {return Tmn;}
double CarbonDioxide::Tmax() {return Tmx;}
char * CarbonDioxide::name() {
 return (char *) m_name.c_str() ;
}
char * CarbonDioxide::formula() {
  return (char *) m_formula.c_str();
}
double CarbonDioxide::MolWt() {return M;}

}



