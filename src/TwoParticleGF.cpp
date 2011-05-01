//
// This file is a part of pomerol - a scientific ED code for obtaining 
// properties of a Hubbard model on a finite-size lattice 
//
// Copyright (C) 2010-2011 Andrey Antipov <antipov@ct-qmc.org>
// Copyright (C) 2010-2011 Igor Krivenko <igor@shg.ru>
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


/** \file src/TwoParticleGF.cpp
** \brief Two-particle Green's function in the Matsubara representation.
**
** \author Igor Krivenko (igor@shg.ru)
** \author Andrey Antipov (antipov@ct-qmc.org)
*/
#include "TwoParticleGF.h"
extern output_handle OUT;
extern std::ostream& OUTPUT_STREAM;

static const Permutation3 permutations3[6] = {
    {{0,1,2},1},
    {{0,2,1},-1},
    {{1,0,2},-1},
    {{1,2,0},1},
    {{2,0,1},1},
    {{2,1,0},-1}
};

TwoParticleGF::TwoParticleGF(StatesClassification& S, Hamiltonian& H,
                AnnihilationOperator& C1, AnnihilationOperator& C2, 
                CreationOperator& CX3, CreationOperator& CX4,
                DensityMatrix& DM) : ComputableObject(), Thermal(beta),
                S(S), H(H), C1(C1), C2(C2), CX3(CX3), CX4(CX4), DM(DM), parts(0)
{
    Storage = new MatsubaraContainer(DM.beta);
    vanish = true;
}

TwoParticleGF::~TwoParticleGF()
{
      for(std::list<TwoParticleGFPart*>::iterator iter = parts.begin(); iter != parts.end(); iter++)
          delete *iter;
}

BlockNumber TwoParticleGF::getLeftIndex(size_t PermutationNumber, size_t OperatorPosition, BlockNumber RightIndex)
{
    switch(permutations3[PermutationNumber].perm[OperatorPosition]){
        case 0: return C1.getLeftIndex(RightIndex);
        case 1: return C2.getLeftIndex(RightIndex);
        case 2: return CX3.getLeftIndex(RightIndex);
        default: return ERROR_BLOCK_NUMBER;
    }
}

BlockNumber TwoParticleGF::getRightIndex(size_t PermutationNumber, size_t OperatorPosition, BlockNumber LeftIndex)
{
    switch(permutations3[PermutationNumber].perm[OperatorPosition]){
        case 0: return C1.getRightIndex(LeftIndex);
        case 1: return C2.getRightIndex(LeftIndex);
        case 2: return CX3.getRightIndex(LeftIndex);
        default: return ERROR_BLOCK_NUMBER;
    }
}

FieldOperatorPart& TwoParticleGF::OperatorPartAtPosition(size_t PermutationNumber, size_t OperatorPosition, BlockNumber LeftIndex)
{
    switch(permutations3[PermutationNumber].perm[OperatorPosition]){
        case 0: return C1.getPartFromLeftIndex(LeftIndex);
        case 1: return C2.getPartFromLeftIndex(LeftIndex);
        case 2: return CX3.getPartFromLeftIndex(LeftIndex);
        default: assert(0);
    }
}

void TwoParticleGF::prepare(void)
{
if (Status < Prepared)
{
    // Find out non-trivial blocks of CX4.
    std::list<BlockMapping> CX4NontrivialBlocks = CX4.getNonTrivialIndices();
    for(std::list<BlockMapping>::const_iterator outer_iter = CX4NontrivialBlocks.begin();
        outer_iter != CX4NontrivialBlocks.end(); outer_iter++){ // Iterate over the outermost index.
            for(size_t p=0; p<6; ++p){ // Choose a permutation
                  BlockNumber LeftIndices[4];
                  LeftIndices[0] = outer_iter->second;
                  LeftIndices[3] = outer_iter->first;
                  LeftIndices[2] = getLeftIndex(p,2,LeftIndices[3]);
                  LeftIndices[1] = getRightIndex(p,0,LeftIndices[0]);
                  // < LeftIndices[0] | O_1 | LeftIndices[1] >
                  // < LeftIndices[1] | O_2 | getRightIndex(p,1,LeftIndices[1]) >
                  // < LeftIndices[2]| O_3 | LeftIndices[3] >
                  // < LeftIndices[3] | CX4 | LeftIndices[0] >
                  // Select a relevant 'world stripe' (sequence of blocks).
                  if(getRightIndex(p,1,LeftIndices[1]) == LeftIndices[2] && LeftIndices[1].isCorrect() && LeftIndices[2].isCorrect()){
                      // DEBUG
                      /*DEBUG("new part: "  << S.getBlockInfo(LeftIndices[0]) << " " 
                                          << S.getBlockInfo(LeftIndices[1]) << " "
                                          << S.getBlockInfo(LeftIndices[2]) << " "
                                          << S.getBlockInfo(LeftIndices[3]) << " "
                      <<"BlockNumbers part: "  << LeftIndices[0] << " " << LeftIndices[1] << " " << LeftIndices[2] << " " << LeftIndices[3]);
                      */
                      parts.push_back(new TwoParticleGFPart(
                            OperatorPartAtPosition(p,0,LeftIndices[0]),
                            OperatorPartAtPosition(p,1,LeftIndices[1]),
                            OperatorPartAtPosition(p,2,LeftIndices[2]),
                            (CreationOperatorPart&)CX4.getPartFromLeftIndex(LeftIndices[3]),
                            H.part(LeftIndices[0]), H.part(LeftIndices[1]), H.part(LeftIndices[2]), H.part(LeftIndices[3]),
                            DM.part(LeftIndices[0]), DM.part(LeftIndices[1]), DM.part(LeftIndices[2]), DM.part(LeftIndices[3]),
                      permutations3[p]));
                      }
            }
    }  
    if ( parts.size() > 0 ) { 
        vanish = false;
        INFO("TwoParticleGF(" << getIndex(0) << getIndex(1) << getIndex(2) << getIndex(3) << "): " << parts.size() << " parts will be calculated");
        };
    Status = Prepared;
};
}

bool TwoParticleGF::vanishes()
{
return vanish;
}

void TwoParticleGF::compute(long NumberOfMatsubaras)
{
if (Status < Computed){
    Storage->prepare(NumberOfMatsubaras);
    unsigned short perm_num=0;
    #ifndef pomerolOpenMP
    for(std::list<TwoParticleGFPart*>::iterator iter = parts.begin(); iter != parts.end(); iter++)
    {
        // TODO: More elegant output.
        std::cout << static_cast<int>((distance(parts.begin(),iter)*100.0)/parts.size()) << "  " << std::flush;
        (*iter)->compute(NumberOfMatsubaras);
        perm_num = getPermutationNumber((*iter)->getPermutation());
        ResonantTerms[perm_num].insert(ResonantTerms[perm_num].end(),(*iter)->getResonantTerms().begin(), (*iter)->getResonantTerms().end());
        NonResonantTerms[perm_num].insert(NonResonantTerms[perm_num].end(),(*iter)->getNonResonantTerms().begin(), (*iter)->getNonResonantTerms().end());
        (*iter)->clear();
    };
    #else
    std::vector<TwoParticleGFPart*> VectorOfParts;
    for(std::list<TwoParticleGFPart*>::iterator iter = parts.begin(); iter != parts.end(); iter++) VectorOfParts.push_back(*iter);

    #pragma omp parallel for
    for(unsigned int i=0; i<VectorOfParts.size(); ++i)
    {
        std::cout << static_cast<int>((i*100.0)/parts.size()) << "  " << std::flush;
        VectorOfParts[i]->compute(NumberOfMatsubaras);
        perm_num = getPermutationNumber(VectorOfParts[i]->getPermutation());
        ResonantTerms[perm_num].insert(ResonantTerms[perm_num].end(),VectorOfParts[i]->getResonantTerms().begin(), VectorOfParts[i]->getResonantTerms().end());
        NonResonantTerms[perm_num].insert(NonResonantTerms[perm_num].end(),VectorOfParts[i]->getNonResonantTerms().begin(), VectorOfParts[i]->getNonResonantTerms().end());
        VectorOfParts[i]->clear();
    }

    #endif
    INFO(std::endl << "TwoParticleGF(" << getIndex(0) << getIndex(1) << getIndex(2) << getIndex(3) << "): substituting values over Matsubara frequencies ... ")
    for (unsigned short i=0; i<6; i++) {
        INFO_NONEWLINE(i+1 << "/6 : " << NonResonantTerms[i].size() << " nonresonant terms and " << ResonantTerms[i].size() << " resonant terms reduced to ");
        TwoParticleGFPart::reduceTerms(TwoParticleGFPart::MultiTermCoefficientTolerance/NonResonantTerms[i].size(), 
                                       TwoParticleGFPart::MultiTermCoefficientTolerance/ResonantTerms[i].size(), 
                                       NonResonantTerms[i],ResonantTerms[i]);
        INFO( NonResonantTerms[i].size() << " nonresonant terms and " << ResonantTerms[i].size() << " resonant terms");
        Storage->fill(NonResonantTerms[i],ResonantTerms[i],permutations3[i]);
        };
    INFO("Done." << std::endl);
    Status = Computed;
    }
}

size_t TwoParticleGF::getNumResonantTerms() const
{
    size_t num = 0;
    for(std::list<TwoParticleGFPart*>::const_iterator iter = parts.begin(); iter != parts.end(); iter++)
        num += (*iter)->getNumResonantTerms();
    return num;
}

size_t TwoParticleGF::getNumNonResonantTerms() const
{
    size_t num = 0;
    for(std::list<TwoParticleGFPart*>::const_iterator iter = parts.begin(); iter != parts.end(); iter++)
        num += (*iter)->getNumNonResonantTerms();
    return num;
}

ComplexType TwoParticleGF::operator()(long MatsubaraNumber1, long MatsubaraNumber2, long MatsubaraNumber3)
{
    ComplexType Value = 0;
    for(std::list<TwoParticleGFPart*>::iterator iter = parts.begin(); iter != parts.end(); iter++){
        Value+=(**iter)(MatsubaraNumber1, MatsubaraNumber2, MatsubaraNumber3 );
        };
    return (*Storage)(MatsubaraNumber1,MatsubaraNumber2,MatsubaraNumber3)+Value;

}

ParticleIndex TwoParticleGF::getIndex(size_t Position) const
{
    switch(Position){
        case 0: return C1.getIndex();
        case 1: return C2.getIndex();
        case 2: return CX3.getIndex();
        case 3: return CX4.getIndex();
        default: assert(0);
    }
}

unsigned short TwoParticleGF::getPermutationNumber ( const Permutation3& in )
{
    for (unsigned short i=0; i<6; ++i) if (in == permutations3[i]) return i;
    ERROR("TwoParticleGF: Permutation " << in << " not found in all permutations3");
    return 0;
}
