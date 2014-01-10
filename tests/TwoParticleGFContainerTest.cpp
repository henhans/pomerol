//
// This file is a part of pomerol - a scientific ED code for obtaining 
// properties of a Hubbard model on a finite-size lattice 
//
// Copyright (C) 2010-2012 Andrey Antipov <antipov@ct-qmc.org>
// Copyright (C) 2010-2012 Igor Krivenko <igor@shg.ru>
//
// pomerol is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pomerol is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pomerol.  If not, see <http://www.gnu.org/licenses/>.

/** \file tests/gamma4.cpp
** \brief Test of a Green's function calculation (1 s-orbital).
**
** \author Igor Krivenko (igor@shg.ru)
*/

#include "Misc.h"
#include "Lattice.h"
#include "LatticePresets.h"
#include "Index.h"
#include "IndexClassification.h"
#include "Operator.h"
#include "OperatorPresets.h"
#include "IndexHamiltonian.h"
#include "Symmetrizer.h"
#include "StatesClassification.h"
#include "HamiltonianPart.h"
#include "Hamiltonian.h"
#include "FieldOperatorContainer.h"
#include "GFContainer.h"
#include "TwoParticleGFContainer.h"
#include "Vertex4Container.h"

#include<cstdlib>

using namespace Pomerol;

RealType U;
RealType beta;

bool compare(ComplexType a, ComplexType b)
{
    return abs(a-b) < 1e-10;
}

RealType delta(int n1, int n2)
{
    return (n1 == n2 ? 1.0 : 0.0);
}

RealType deltam(int n1, int n2)
{
    return (n1+n2==-1 ? 1.0 : 0.0);
}

inline RealType w(int n)
{
    return M_PI*(2*n+1)/beta;
}

// Reference gamma4(up,up,up,up)
ComplexType gamma4ref_uuuu(int n1, int n2, int n3)
{
    ComplexType Value = 0;

    RealType omega1 = w(n1);
    RealType omega2 = w(n2);
    RealType omega3 = w(n3);

    Value = -beta*(delta(n1,n3)-delta(n2,n3))*sqr(0.5*U)*
            (1. + sqr(0.5*U/omega1))*
            (1. + sqr(0.5*U/omega2));

    return Value;
}

// Reference gamma4(up,down,up,down)
ComplexType gamma4ref_udud(int n1, int n2, int n3)
{
    ComplexType Value = 0;

    RealType omega1 = w(n1);
    RealType omega2 = w(n2);
    RealType omega3 = w(n3);
    RealType omega4 = omega1+omega2-omega3;

    RealType w = 1.0/(1.0+exp(beta*0.5*U));

    Value += U;
    Value += -0.125*U*U*U*(sqr(omega1)+sqr(omega2)+sqr(omega3)+sqr(omega4))/(omega1*omega2*omega3*omega4);
    Value += -0.1875*U*U*U*U*U/(omega1*omega2*omega3*omega4);
    Value += -beta*(2*deltam(n1,n2)+delta(n1,n3))*w*sqr(0.5*U)*(1.0+sqr(0.5*U/omega2))*(1.0+sqr(0.5*U/omega3));
    Value += beta*(2*delta(n2,n3)+delta(n1,n3))*(1-w)*sqr(0.5*U)*(1.0+sqr(0.5*U/omega1))*(1.0+sqr(0.5*U/omega2));

    return Value;
}

int main(int argc, char* argv[])
{
    boost::mpi::environment env(argc,argv);
    boost::mpi::communicator world;

    U = 1.0;

    
    Lattice L;
    L.addSite(new Lattice::Site("A",1,2));
    LatticePresets::addCoulombS(&L, "A", U, -U/2.);

    IndexClassification IndexInfo(L.getSiteMap());
    IndexInfo.prepare();
    IndexInfo.printIndices();

    IndexHamiltonian Storage(&L,IndexInfo);
    Storage.prepare();

    Symmetrizer Symm(IndexInfo, Storage);
    Symm.compute();

    StatesClassification S(IndexInfo,Symm);
    S.compute();

    Hamiltonian H(IndexInfo, Storage, S);
    H.prepare();
    H.compute();

    srand(time(NULL));
    beta = 40.0 ;//+ 10.0*RealType(rand())/RAND_MAX;

    DensityMatrix rho(S,H,beta);
    rho.prepare();
    rho.compute();

    FieldOperatorContainer Operators(IndexInfo, S, H);
    Operators.prepareAll();
    Operators.computeAll();

    GFContainer G(IndexInfo,S,H,rho,Operators);
    G.prepareAll();
    G.computeAll();

    TwoParticleGFContainer Chi(IndexInfo,S,H,rho,Operators);
    Chi.prepareAll();
    Chi.computeAll();

    bool success = true; 
    int wn = 4;

    INFO("TEST: CHI_UPUPUPUP");
    auto &GF = G(0,0);
    auto &Chi_uuuu = Chi(IndexCombination4(0,0,0,0));
    for(int n1 = -wn; n1<wn; ++n1)
    for(int n2 = -wn; n2<wn; ++n2)
    for(int n3 = -wn; n3<wn; ++n3){
         int n4 = n1+n2-n3;
         auto l = Chi_uuuu(n1,n2,n3); 
         auto r = gamma4ref_uuuu(n1,n2,n3)*GF(n1)*GF(n2)*GF(n3)*GF(n4) + beta*GF(n1)*GF(n2)*delta(n1,n4) - beta*GF(n1)*GF(n2)*delta(n1,n3);
//         INFO(n1 << " " << n2 << " " << n3 << " " << n1+n2-n3 << " : " << l << " == " << r);
         success = success && compare(l,r);
     }
    world.barrier();
    INFO("SUCCESS");
    if (!success) return EXIT_FAILURE;
    world.barrier();


    INFO("TEST: CHI_UPDOWNUPDOWN");
    auto &Chi_udud = Chi(IndexCombination4(0,1,0,1));
    for(int n1 = -wn; n1<wn; ++n1)
    for(int n2 = -wn; n2<wn; ++n2)
    for(int n3 = -wn; n3<wn; ++n3){
         int n4 = n1+n2-n3;
         auto l = Chi_udud(n1,n2,n3); 
         auto r = gamma4ref_udud(n1,n2,n3)*GF(n1)*GF(n2)*GF(n3)*GF(n4) + beta*GF(n1)*GF(n2)*delta(n1,n4) - beta*GF(n1)*GF(n2)*delta(n1,n3);
//         INFO(n1 << " " << n2 << " " << n3 << " " << n1+n2-n3 << " : " << l << " == " << r);
         success = success && compare(l,r);
     }
    world.barrier();
    INFO("SUCCESS");
    if (!success) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
