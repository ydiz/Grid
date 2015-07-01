#include <Grid.h>

using namespace std;
using namespace Grid;
using namespace Grid::QCD;

int main (int argc, char ** argv)
{
  Grid_init(&argc,&argv);

  const int Ls=8;

  GridCartesian         * UGrid   = SpaceTimeGrid::makeFourDimGrid(GridDefaultLatt(), GridDefaultSimd(Nd,vComplex::Nsimd()),GridDefaultMpi());
  GridRedBlackCartesian * UrbGrid = SpaceTimeGrid::makeFourDimRedBlackGrid(UGrid);

  GridCartesian         * FGrid   = SpaceTimeGrid::makeFiveDimGrid(Ls,UGrid);
  GridRedBlackCartesian * FrbGrid = SpaceTimeGrid::makeFiveDimRedBlackGrid(Ls,UGrid);

  ///////////////////////////////////////////////////
  // Construct a coarsened grid; utility for this?
  ///////////////////////////////////////////////////
  std::vector<int> clatt = GridDefaultLatt();
  for(int d=0;d<clatt.size();d++){
    clatt[d] = clatt[d]/2;
  }
  GridCartesian *Coarse4d =  SpaceTimeGrid::makeFourDimGrid(clatt, GridDefaultSimd(Nd,vComplex::Nsimd()),GridDefaultMpi());;
  GridCartesian *Coarse5d =  SpaceTimeGrid::makeFiveDimGrid(1,Coarse4d);

  std::vector<int> seeds4({1,2,3,4});
  std::vector<int> seeds5({5,6,7,8});
  std::vector<int> cseeds({5,6,7,8});
  GridParallelRNG          RNG5(FGrid);   RNG5.SeedFixedIntegers(seeds5);
  GridParallelRNG          RNG4(UGrid);   RNG4.SeedFixedIntegers(seeds4);
  GridParallelRNG          CRNG(Coarse5d);CRNG.SeedFixedIntegers(cseeds);

  LatticeFermion    src(FGrid); gaussian(RNG5,src);
  LatticeFermion result(FGrid); result=zero;
  LatticeFermion    ref(FGrid); ref=zero;
  LatticeFermion    tmp(FGrid);
  LatticeFermion    err(FGrid);
  LatticeGaugeField Umu(UGrid); 

  NerscField header;
  std::string file("./ckpoint_lat.400");
  readNerscConfiguration(Umu,header,file);

  //  SU3::ColdConfiguration(RNG4,Umu);
  //  SU3::TepidConfiguration(RNG4,Umu);
  //  SU3::HotConfiguration(RNG4,Umu);
  //  Umu=zero;

  RealD mass=0.1;
  RealD M5=1.5;

  std::cout << "**************************************************"<< std::endl;
  std::cout << "Building g5R5 hermitian DWF operator" <<std::endl;
  std::cout << "**************************************************"<< std::endl;
  DomainWallFermion Ddwf(Umu,*FGrid,*FrbGrid,*UGrid,*UrbGrid,mass,M5);
  Gamma5R5HermitianLinearOperator<DomainWallFermion,LatticeFermion> HermIndefOp(Ddwf);

  const int nbasis = 8;

  typedef Aggregation<vSpinColourVector,vTComplex,nbasis>     Subspace;
  typedef CoarsenedMatrix<vSpinColourVector,vTComplex,nbasis> LittleDiracOperator;
  typedef LittleDiracOperator::CoarseVector                   CoarseVector;

  std::cout << "**************************************************"<< std::endl;
  std::cout << "Calling Aggregation class to build subspace" <<std::endl;
  std::cout << "**************************************************"<< std::endl;
  MdagMLinearOperator<DomainWallFermion,LatticeFermion> HermDefOp(Ddwf);
  Subspace Aggregates(Coarse5d,FGrid);
  Aggregates.CreateSubspace(RNG5,HermDefOp);


  LittleDiracOperator LittleDiracOp(*Coarse5d);
  LittleDiracOp.CoarsenOperator(FGrid,HermIndefOp,Aggregates);
  
  CoarseVector c_src (Coarse5d);
  CoarseVector c_res (Coarse5d);
  gaussian(CRNG,c_src);
  c_res=zero;

  std::cout << "**************************************************"<< std::endl;
  std::cout << "Solving mdagm-CG on coarse space "<< std::endl;
  std::cout << "**************************************************"<< std::endl;
  MdagMLinearOperator<LittleDiracOperator,CoarseVector> PosdefLdop(LittleDiracOp);
  ConjugateGradient<CoarseVector> CG(1.0e-6,10000);
  CG(PosdefLdop,c_src,c_res);

  std::cout << "**************************************************"<< std::endl;
  std::cout << "Solving indef-MCR on coarse space "<< std::endl;
  std::cout << "**************************************************"<< std::endl;
  HermitianLinearOperator<LittleDiracOperator,CoarseVector> HermIndefLdop(LittleDiracOp);
  ConjugateResidual<CoarseVector> MCR(1.0e-6,10000);
  MCR(HermIndefLdop,c_src,c_res);

  std::cout << "**************************************************"<< std::endl;
  std::cout << "Done "<< std::endl;
  std::cout << "**************************************************"<< std::endl;
  Grid_finalize();
}
