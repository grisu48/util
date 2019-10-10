/* =============================================================================
 * 
 * Title:         Numeric test program
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2019 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * 
 * =============================================================================
 */
 

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "numeric.hpp"

using namespace std;
using namespace numeric;

#define N1 20
#define N2 30
#define N3 10
#define N4 40

static long euler_sum(const long n) {
	return n*(n+1L)/2L;
}


/* ==== Tests that should be implemented using gtest ======================== */

static void test_array() {
	Array<double> arr(N1);
	for(int i=0;i<N1;i++) arr[i] = i;
	if(arr.sum() != euler_sum(N1-1)) {
		cerr << "N1 sum failed: " << arr.sum() << "!=" << euler_sum(N1-1) << endl;
		exit(EXIT_FAILURE);
	}
	arr.resize(N2);
	for(int i=N1;i<N2;i++) arr[i] = i;
	if(arr.sum() != euler_sum(N2-1)) {
		cerr << "N2 sum failed: " << arr.sum() << "!=" << euler_sum(N2-1) << endl;
		exit(EXIT_FAILURE);
	}
	arr.resize(N3);
	if(arr.sum() != euler_sum(N3-1)) {
		cerr << "N3 sum failed: " << arr.sum() << "!=" << euler_sum(N3-1) << endl;
		exit(EXIT_FAILURE);
	}
}

static void test_cube() {
	Cube<double> c(N1,N2,N3);
	c = 1.0;
	if(c.sum() != (N1*N2*N3)) {
		cerr << "Cube initialisation error :" << c.sum() << " != " << (N1*N2*N3) << endl;
		exit(EXIT_FAILURE);
	}
	double c_sum = 0;
	for(int i=0;i<N1;i++) {
		for(int j=0;j<N2;j++) {
			for(int k=0;k<N3;k++) {
				c(i,j,k) = i*j*k;
				c_sum += i*j*k;
			}
		}
	}
	if(c.sum() != c_sum) {
		cerr << "Cube sum error :" << c.sum() << " != " << c_sum << endl;
		exit(EXIT_FAILURE);
	}
}

static void test_tesseract() {
    Tesseract<double> t(N1,N2,N3,N4);
    t = 0.0;
    if(t.sum() != 0.0) {
		cerr << "Tesseract initialisation error :" << t.sum() << " != 0 after clearing out" << endl;
		exit(EXIT_FAILURE);
    }
    t = 1.0;
	if(t.sum() != (N1*N2*N3*N4)) {
		cerr << "Tesseract first sum error :" << t.sum() << " != " << (N1*N2*N3*N4) << endl;
		exit(EXIT_FAILURE);
	}
	
	double t_sum = 0;
	for(int i=0;i<N1;i++) {
		for(int j=0;j<N2;j++) {
			for(int k=0;k<N3;k++) {
				for(int l=0;l<N4;l++) {
					t(i,j,k,l) = i*j*k*l;
					t_sum += i*j*k*l;
				}
			}
		}
	}
	if(t.sum() != t_sum) {
		cerr << "Tesseract sum error :" << t.sum() << " != " << t_sum << endl;
		exit(EXIT_FAILURE);
	}
}







int main() { //int argc, char** argv) {
	test_array();
	test_cube();
    test_tesseract();

    cout << "All good" << endl;
    return EXIT_SUCCESS;
}
