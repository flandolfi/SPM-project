/* ***************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License version 3 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************


 Author: Tiziano De Matteis <dematteis@di.unipi.it>
*/

/**
  Quicksort: sort an array of N integer with the DAC pattern

  Author: Tiziano De Matteis <dematteis@di.unipi.it>
  */

#include <iostream>
#include <functional>
#include <algorithm>
#include "../includes/utils.h"
#if USE_FF
#include <ff/dc.hpp>
using namespace ff;
#elif USE_OMP
#include "../includes/dac_openmp.hpp"
#elif USE_TBB
#include "../includes/dac_tbb.hpp"
#else
#include <dac/dac.h>
#endif
using namespace std;
#define CUTOFF 2000

// Operand (i.e. the Problem) and Results share the same format
struct ops{
    int *array=nullptr;          //array to sort
    int left=0;                     //left index
    int right=0;                    //right index

};

//for mergesort we have the same type
typedef struct ops Operand;
typedef struct ops Result;



/*
 * The divide chooses as pivot the middle element and redistributes the elements
 */
void divide(const Operand &op, std::vector<Operand> &ops)
{
    ops.push_back(Operand());
    ops.push_back(Operand());

    int *a=op.array;
    int pivot=a[(op.left+op.right)/2];
    int i = op.left-1, j = op.right+1;
    int tmp;

    while(true)
    {
        do{
            i++;
        }while(a[i]<pivot);
        do{
            j--;
        }while(a[j]>pivot);

        if(i>=j)
           break;

        //swap
        tmp=a[i];
        a[i]=a[j];
        a[j]=tmp;
    }

    //j is the pivot

    ops[0].array=a;
    ops[0].left=op.left;
    ops[0].right=j;

    ops[1].array=a;
    ops[1].left=j+1;
    ops[1].right=op.right;
}


/*
 * The Combine does nothing
 */
void mergeQS(vector<Result> &ress, Result &ret)
{
    ret.array=ress[0].array;
    ret.left=ress[0].left;
    ret.right=ress[1].right;
}


/*
 * Base case: we resort on std::sort
 */
void seq(const Operand &op, Result &ret)
{

    std::sort(&(op.array[op.left]),&(op.array[op.right+1]));

	//build result
    ret.array=op.array;
    ret.left=op.left;
    ret.right=op.right;
}

/*
 * Base case condition
 */
bool cond(const Operand &op)
{
	return (op.right-op.left<=CUTOFF);
}

int main(int argc, char *argv[])
{
    if(argc<5)
    {
        cerr << "Usage: " << argv[0] << " <num_elements> <min_proc> <max_proc> <num_trials>" << endl;
        exit(-1);
    }
    const std::function<void(const Operand&,vector<Operand>&)> div(divide);
    const std::function<void(const Operand &,Result &)> sq(seq);
    const std::function<void(vector<Result>&,Result &)> mergef(mergeQS);
    const std::function<bool(const Operand &)> cf(cond);

    int num_elem=atoi(argv[1]);
    int min_proc=atoi(argv[2])  ;
    int max_proc=atoi(argv[3]);
    int num_trials=atoi(argv[4]);


    printf("Workers,Time (ms)\n");

    for (auto nwork = min_proc; nwork <= max_proc; nwork *= 2) {
        for (auto trial = 0; trial < num_trials; trial++) {
            int *numbers=generateRandomArray(num_elem);

            //build the operand
            //Operand *op=new Operand();
            Operand op;
            op.array=numbers;
            op.left=0;
            op.right=num_elem-1;
            Result res;
#if USE_FF
            ff_DC<Operand, Result> dac(div,mergef,sq,cf,op,res,nwork);
#elif USE_OMP
            DacOpenmp<Operand, Result> dac(div,mergef,sq,cf,op,res,nwork);
#elif USE_TBB
            DacTBB<Operand, Result> dac(div,mergef,sq,cf,op,res,nwork);
#else
            DAC<Operand, Result> dac(div, mergef, cf, sq);
#endif

            long start_t=current_time_usecs();

            //compute
#if USE_FF
            dac.run_and_wait_end();
#elif USE_OMP
            dac.compute();
#elif USE_TBB
            dac.compute();
#else
            dac.compute(op, res, nwork);
#endif
            long end_t=current_time_usecs();

            // correctness check
            if(!isArraySorted(numbers,num_elem))
            {
                fprintf(stderr,"Error: array is not sorted!!\n");
                exit(-1);
            }

            printf("%d,%ld\n",nwork, end_t-start_t);
        }
    }

    return 0;
}

