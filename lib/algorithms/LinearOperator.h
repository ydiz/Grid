#ifndef  GRID_ALGORITHM_LINEAR_OP_H
#define  GRID_ALGORITHM_LINEAR_OP_H

#include <Grid.h>

namespace Grid {

  /////////////////////////////////////////////////////////////////////////////////////////////
  // Interface defining what I expect of a general sparse matrix, such as a Fermion action
  /////////////////////////////////////////////////////////////////////////////////////////////
    template<class Field> class SparseMatrixBase {
    public:
      // Full checkerboar operations
      virtual void M    (const Field &in, Field &out);
      virtual void Mdag (const Field &in, Field &out);
      virtual RealD MdagM(const Field &in, Field &out);
      
      // half checkerboard operaions
      virtual void Mpc      (const Field &in, Field &out);
      virtual void MpcDag   (const Field &in, Field &out);
      virtual RealD MpcDagMpc(const Field &in, Field &out);
    };

  /////////////////////////////////////////////////////////////////////////////////////////////
  // LinearOperators Take a something and return a something.
  /////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Hopefully linearity is satisfied and the AdjOp is indeed the Hermitian conjugate (transpose if real):
  //
  //   i)  F(a x + b y) = aF(x) + b F(y).
  //  ii)  <x|Op|y> = <y|AdjOp|x>^\ast
  //
  // Would be fun to have a test linearity & Herm Conj function!
  /////////////////////////////////////////////////////////////////////////////////////////////
    template<class Field> class LinearOperatorBase {
    public:
      virtual void Op     (const Field &in, Field &out) = 0; // Abstract base
      virtual void AdjOp  (const Field &in, Field &out) = 0; // Abstract base
    };

  /////////////////////////////////////////////////////////////////////////////////////////////
  // Hermitian operators are self adjoint and only require Op to be defined, so refine the base
  /////////////////////////////////////////////////////////////////////////////////////////////
    template<class Field> class HermitianOperatorBase : public LinearOperatorBase<Field> {
    public:
      virtual RealD OpAndNorm(const Field &in, Field &out);
      void AdjOp(const Field &in, Field &out) {
	Op(in,out);
      };
      void Op(const Field &in, Field &out) {
	OpAndNorm(in,out);
      };
    };

  /////////////////////////////////////////////////////////////////////////////////////////////
  // Whereas non hermitian takes a generic sparse matrix (e.g. lattice action)
  // conforming to sparse matrix interface and builds the full checkerboard non-herm operator
  // Op and AdjOp distinct.
  // By sharing the class for Sparse Matrix across multiple operator wrappers, we can share code
  // between RB and non-RB variants. Sparse matrix is like the fermion action def, and then
  // the wrappers implement the specialisation of "Op" and "AdjOp" to the cases minimising
  // replication of code.
  /////////////////////////////////////////////////////////////////////////////////////////////
    template<class SparseMatrix,class Field>
    class NonHermitianOperator : public LinearOperatorBase<Field> {
      SparseMatrix &_Mat;
    public:
      NonHermitianOperator(SparseMatrix &Mat): _Mat(Mat){};
      void Op     (const Field &in, Field &out){
	_Mat.M(in,out);
      }
      void AdjOp     (const Field &in, Field &out){
	_Mat.Mdag(in,out);
      }
    };
    
    ////////////////////////////////////////////////////////////////////////////////////
    // Redblack Non hermitian wrapper
    ////////////////////////////////////////////////////////////////////////////////////
    template<class SparseMatrix,class Field>
    class NonHermitianRedBlackOperator : public LinearOperatorBase<Field> {
      SparseMatrix &_Mat;
    public:
      NonHermitianRedBlackOperator(SparseMatrix &Mat): _Mat(Mat){};
      void Op     (const Field &in, Field &out){
	_Mat.Mpc(in,out);
      }
      void AdjOp     (const Field &in, Field &out){ //
	_Mat.MpcDag(in,out);
      }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    // Hermitian wrapper
    ////////////////////////////////////////////////////////////////////////////////////
    template<class SparseMatrix,class Field>
    class HermitianOperator : public HermitianOperatorBase<Field> {
      SparseMatrix &_Mat;
    public:
      HermitianOperator(SparseMatrix &Mat): _Mat(Mat) {};
      RealD OpAndNorm(const Field &in, Field &out){
	return _Mat.MdagM(in,out);
      }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    // Hermitian RedBlack wrapper
    ////////////////////////////////////////////////////////////////////////////////////
    template<class SparseMatrix,class Field>
    class HermitianRedBlackOperator : public HermitianOperatorBase<Field> {
      SparseMatrix &_Mat;
    public:
      HermitianRedBlackOperator(SparseMatrix &Mat): _Mat(Mat) {};
      RealD OpAndNorm(const Field &in, Field &out){
	return _Mat.MpcDagMpc(in,out);
      }
    };


    /////////////////////////////////////////////////////////////
    // Base classes for functions of operators
    /////////////////////////////////////////////////////////////
    template<class Field> class OperatorFunction {
    public:
      virtual void operator() (LinearOperatorBase<Field> *Linop, const Field &in, Field &out) = 0;
    };

    /////////////////////////////////////////////////////////////
    // Base classes for polynomial functions of operators ? needed?
    /////////////////////////////////////////////////////////////
    template<class Field> class OperatorPolynomial : public OperatorFunction<Field> {
    public:
      virtual void operator() (LinearOperatorBase<Field> *Linop,const Field &in, Field &out) = 0;
    };

    /////////////////////////////////////////////////////////////
    // Base classes for iterative processes based on operators
    // single input vec, single output vec.
    /////////////////////////////////////////////////////////////
    template<class Field> class IterativeProcess : public OperatorFunction<Field> {
    public:
      RealD   Tolerance;
      Integer MaxIterations;
      IterativeProcess(RealD tol,Integer maxit) : Tolerance(tol),MaxIterations(maxit) {};
      virtual void operator() (LinearOperatorBase<Field> *Linop,const Field &in, Field &out) = 0;
    };

    /////////////////////////////////////////////////////////////
    // Grand daddy iterative method
    /////////////////////////////////////////////////////////////
    template<class Field> class ConjugateGradient : public IterativeProcess<Field> {
    public:
      virtual void operator() (HermitianOperatorBase<Field> *Linop,const Field &in, Field &out) = 0;
    };

    /////////////////////////////////////////////////////////////
    // A little more modern
    /////////////////////////////////////////////////////////////
    template<class Field> class PreconditionedConjugateGradient : public  IterativeProcess<Field>  {
    public:
      void operator() (HermitianOperatorBase<Field> *Linop,
		       OperatorFunction<Field> *Preconditioner,
		       const Field &in, 
		       Field &out) = 0;
    };

    // FIXME : To think about

    // Chroma functionality list defining LinearOperator
    /*
     virtual void operator() (T& chi, const T& psi, enum PlusMinus isign) const = 0;
     virtual void operator() (T& chi, const T& psi, enum PlusMinus isign, Real epsilon) const
     virtual const Subset& subset() const = 0;
     virtual unsigned long nFlops() const { return 0; }
     virtual void deriv(P& ds_u, const T& chi, const T& psi, enum PlusMinus isign) const
     class UnprecLinearOperator : public DiffLinearOperator<T,P,Q>
       const Subset& subset() const {return all;}
     };
    */

    // Chroma interface defining GaugeAction
    /*
      template<typename P, typename Q>   class GaugeAction
  virtual const CreateGaugeState<P,Q>& getCreateState() const = 0;
  virtual GaugeState<P,Q>* createState(const Q& q) const
  virtual const GaugeBC<P,Q>& getGaugeBC() const
  virtual const Set& getSet(void) const = 0;
  virtual void deriv(P& result, const Handle< GaugeState<P,Q> >& state) const 
  virtual Double S(const Handle< GaugeState<P,Q> >& state) const = 0;

  class LinearGaugeAction : public GaugeAction< multi1d<LatticeColorMatrix>, multi1d<LatticeColorMatrix> >
  typedef multi1d<LatticeColorMatrix>  P;
  typedef multi1d<LatticeColorMatrix>  Q;
  virtual void staple(LatticeColorMatrix& result,
		      const Handle< GaugeState<P,Q> >& state,
		      int mu, int cb) const = 0;
    */

    // Chroma interface defining FermionAction
    /*
     template<typename T, typename P, typename Q>  class FermAct4D : public FermionAction<T,P,Q>
     virtual LinearOperator<T>* linOp(Handle< FermState<T,P,Q> > state) const = 0;
     virtual LinearOperator<T>* lMdagM(Handle< FermState<T,P,Q> > state) const = 0;
     virtual LinOpSystemSolver<T>* invLinOp(Handle< FermState<T,P,Q> > state,
     virtual MdagMSystemSolver<T>* invMdagM(Handle< FermState<T,P,Q> > state,
     virtual LinOpMultiSystemSolver<T>* mInvLinOp(Handle< FermState<T,P,Q> > state,
     virtual MdagMMultiSystemSolver<T>* mInvMdagM(Handle< FermState<T,P,Q> > state,
     virtual MdagMMultiSystemSolverAccumulate<T>* mInvMdagMAcc(Handle< FermState<T,P,Q> > state,
     virtual SystemSolver<T>* qprop(Handle< FermState<T,P,Q> > state,
     class DiffFermAct4D : public FermAct4D<T,P,Q>
     virtual DiffLinearOperator<T,Q,P>* linOp(Handle< FermState<T,P,Q> > state) const = 0;
     virtual DiffLinearOperator<T,Q,P>* lMdagM(Handle< FermState<T,P,Q> > state) const = 0;
    */
}

#endif
