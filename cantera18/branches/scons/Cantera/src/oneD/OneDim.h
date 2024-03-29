/**
 * @file OneDim.h
 */

#ifndef CT_ONEDIM_H
#define CT_ONEDIM_H

#include "Domain1D.h"

namespace Cantera {

    class MultiJac;
    class MultiNewton;

    /**
     * Container class for multiple-domain 1D problems. Each domain is
     * represented by an instance of Domain1D.
     */
    class OneDim {

    public: 

        // Default constructor.
        OneDim();

        // Constructor.
        OneDim(std::vector<Domain1D*> domains);

        /// Destructor.
        virtual ~OneDim();

        /// Add a domain.
        void addDomain(Domain1D* d);

        /// Return a reference to the Jacobian evaluator.
        MultiJac& jacobian();

        /// Return a reference to the Newton iterator.
        MultiNewton& newton();

        /**
         * Solve F(x) = 0, where F(x) is the multi-domain residual function.
         * @param x0         Starting estimate of solution.
         * @param x1         Final solution satisfying F(x1) = 0.
         * @param loglevel   Controls amount of diagnostic output.
         */
        int solve(doublereal* x0, doublereal* x1, int loglevel);

        /// Number of domains.
        size_t nDomains() const { return m_nd; }

        /// Return a reference to domain i.
        Domain1D& domain(size_t i) const { return *m_dom[i]; }

        size_t domainIndex(std::string name);

        /// The index of the start of domain i in the solution vector.
        size_t start(size_t i) const { return m_dom[i]->loc(); }

        /// Total solution vector length;
        size_t size() const { return m_size; }

        /// Pointer to left-most domain (first added).
        Domain1D* left() { return m_dom[0]; }

        /// Pointer to right-most domain (last added).
        Domain1D* right() { return m_dom.back(); }

        /// Number of solution components at global point jg.
        size_t nVars(size_t jg) { return m_nvars[jg]; }

        /**
         * Location in the solution vector of the first component of
         *  global point jg.
         */
        size_t loc(size_t jg) { return m_loc[jg]; }

        /// Jacobian bandwidth.
        size_t bandwidth() const { return m_bw; }

        /// Initialize.
        void init();

        /// Total number of points.
        size_t points() { return m_pts; }

        /**
         * Steady-state max norm of the residual evaluated using solution x.
         * On return, array r contains the steady-state residual values.
         */
        doublereal ssnorm(doublereal* x, doublereal* r);

        /// Reciprocal of the time step. 
        doublereal rdt() const { return m_rdt; }
        
        /// Prepare for time stepping beginning with solution x.
        void initTimeInteg(doublereal dt, doublereal* x);

        /// True if transient mode.
        bool transient() const { return (m_rdt != 0.0);}

        /// True if steady mode.
        bool steady() const { return (m_rdt == 0.0); }


        /**
         * Set steady mode.  After invoking this method, subsequent
         * calls to solve() will solve the steady-state problem.
         */
        void setSteadyMode();


        /**
         * Evaluate the multi-domain residual function
         * 
         * @param j       if j > 0, only evaluate residual for points j-1, j, 
         *                and j + 1; otherwise, evaluate at all grid points.
         * @param x       solution vector
         * @param r       on return, contains the residual vector
         * @param rdt     Reciprocal of the time step. if omitted, then
         *                  the default value is used.
         * @param count   Set to zero to omit this call from the statistics
         */ 
        void eval(size_t j, double* x, double* r, doublereal rdt=-1.0,
            int count = 1);

        /// Pointer to the domain global point i belongs to.
        Domain1D* pointDomain(size_t i);

        void resize();

        //doublereal solveTime() { return m_solve_time; }

        //void setTransientMask();
        vector_int& transientMask() { return m_mask; }

        double timeStep(int nsteps, double dt, double* x, 
            double* r, int loglevel);

        void writeStats();

        void save(std::string fname, std::string id, std::string desc, doublereal* sol);

        // options
        void setMinTimeStep(doublereal tmin) { m_tmin = tmin; }
        void setMaxTimeStep(doublereal tmax) { m_tmax = tmax; }
        void setTimeStepFactor(doublereal tfactor) { m_tfactor = tfactor; }
        void setJacAge(int ss_age, int ts_age=-1) {
            m_ss_jac_age = ss_age;
            if (ts_age > 0) 
                m_ts_jac_age = ts_age;
            else
                m_ts_jac_age = m_ss_jac_age;
        }
        void saveStats();

    protected:

        void evalSSJacobian(doublereal* x, doublereal* xnew);

        doublereal m_tmin;        // minimum timestep size
        doublereal m_tmax;        // maximum timestep size
        doublereal m_tfactor;     // factor time step is multiplied by
                                  // if time stepping fails ( < 1 )

        MultiJac* m_jac;          // Jacobian evaluator
        MultiNewton* m_newt;      // Newton iterator
        doublereal m_rdt;         // reciprocal of time step
        bool m_jac_ok;            // if true, Jacobian is current
        size_t m_nd;                 // number of domains
        size_t m_bw;                 // Jacobian bandwidth
        size_t m_size;               // solution vector size
        
        std::vector<Domain1D*> m_dom, m_connect, m_bulk;

        bool m_init;
        std::vector<size_t> m_nvars;
        std::vector<size_t> m_loc;
        vector_int m_mask;
        size_t m_pts;
        doublereal m_solve_time;

        // options
        int m_ss_jac_age, m_ts_jac_age;

    private:

        // statistics
        int m_nevals;
        doublereal m_evaltime;
        std::vector<size_t> m_gridpts;
        vector_int m_jacEvals;
        vector_fp m_jacElapsed;
        vector_int m_funcEvals;
        vector_fp m_funcElapsed;


    };

}

#endif


