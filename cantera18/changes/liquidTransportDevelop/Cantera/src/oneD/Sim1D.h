/**
 * @file Sim1D.h
 */

#ifndef CT_SIM1D_H
#define CT_SIM1D_H

#ifdef WIN32
#pragma warning(disable:4996)
#endif

#include "OneDim.h"
#include "funcs.h"

namespace Cantera {

    /**
     * One-dimensional simulations. Class Sim1D extends class OneDim
     * by storing the solution vector, and by adding a hybrid
     * Newton/time-stepping solver.
     */
    class Sim1D : public OneDim {

    public: 

        
      //! Default constructor.
      /*!
       *  This constructor is provided to make
       *  the class default-constructible, but is not meant to be
       *  used in most applications.  Use the next constructor
       */
      Sim1D();


        /**
         * Standard constructor. 
         * @param domains A vector of pointers to the domains to be linked together.
         * The domain pointers must be entered in left-to-right order --- i.e., 
         * the pointer to the leftmost domain is domain[0], the pointer to the
         * domain to its right is domain[1], etc. 
         */ 
        Sim1D(std::vector<Domain1D*>& domains);

        /// Destructor. Does nothing.
        virtual ~Sim1D() {}

        /**
         * @name Setting initial values
         *
         * These methods are used to set the initial values of 
         * solution components.
         */
        //@{

        /// Set initial guess based on equilibrium
        //added by Karl Meredith
        void setInitialGuess(std::string component, vector_fp& locs, vector_fp& vals);

        /// Set one entry in the solution vector.
        void setValue(int dom, int comp, int localPoint,  doublereal value);

        /// Get one entry in the solution vector.
        doublereal value(int dom, int comp, int localPoint) const;

        doublereal workValue(int dom, int comp, int localPoint) const;

        /// Specify a profile for one component of one domain.
        void setProfile(int dom, int comp, const vector_fp& pos, 
            const vector_fp& values);

        /// Set component 'comp' of domain 'dom' to value 'v' at all points.
        void setFlatProfile(int dom, int comp, doublereal v);

        //@}

        void save(std::string fname, std::string id, std::string desc);

        /// Print to stream s the current solution for all domains.     
        void showSolution(std::ostream& s);
        void showSolution();

        const doublereal* solution() { return DATA_PTR(m_x); }

        void setTimeStep(doublereal stepsize, int n, integer* tsteps);

        //void setMaxTimeStep(doublereal tmax) { m_maxtimestep = tmax; }

        void solve(int loglevel = 0, bool refine_grid = true);

        void eval(doublereal rdt=-1.0, int count = 1) {
            OneDim::eval(-1, DATA_PTR(m_x), DATA_PTR(m_xnew), rdt, count);
        }

        /// Refine the grid in all domains.
        int refine(int loglevel=0);

        //added by Karl Meredith
        int setFixedTemperature(doublereal t);
        //added by Karl Meredith
        void setAdiabaticFlame(void);

        /// Set the criteria for grid refinement.
        void setRefineCriteria(int dom = -1, doublereal ratio = 10.0,
            doublereal slope = 0.8, doublereal curve = 0.8, doublereal prune = -0.1);
        void setMaxGridPoints(int dom = -1, int npoints = 300);

        void restore(std::string fname, std::string id);
        void getInitialSoln();

        void setSolution(const doublereal* soln) {
            std::copy(soln, soln + m_x.size(), DATA_PTR(m_x));
        }

        const doublereal* solution() const { return DATA_PTR(m_x); }

        doublereal jacobian(int i, int j);

        void evalSSJacobian();

    protected:

        vector_fp m_x;          // the solution vector
        vector_fp m_xnew;       // a work array used to hold the residual 
                                //      or the new solution
        doublereal m_tstep;     // timestep
        vector_int m_steps;     // array of number of steps to take before 
                                //      re-attempting the steady-state solution


    private:

        /// Calls method _finalize in each domain.
        void finalize();

        void newtonSolve(int loglevel);


    };

}
#endif


