/* =============================================================================
 *
 * Title:       Basic numerics class.
 * Author:      Felix Niederwanger
 * License:     MIT (http://opensource.org/licenses/MIT)
 * Description: Numerics, standalone header file
 * =============================================================================
 */

#ifndef _NUMERIC_CLASS_HPP_
#define _NUMERIC_CLASS_HPP_

#include <stdlib.h>
#include <string.h>

namespace numeric {

template <class T>
class Array {
protected:
	T* val;
	size_t n;
	
public:
	Array() : val(NULL),n(0) {}
	Array(const size_t n) : Array() {
		resize(n);
	}
	Array(const Array &src) {
		this->n = src.n;
		this->val = (T*)malloc(n*sizeof(T));
		memcpy(this->val, src.val, n*sizeof(T));
	}
	Array(Array &&src) {
		this->n = src.n;
		this->val = src.val;
		src.n = 0;
		src.val = NULL;
	}
	virtual ~Array() { if(this->val != NULL) free(val); }

	size_t size() const { return this->n; }

	/** Resize the given array, conserving it's internal data */
	void resize(const size_t n) {
		if(val == NULL) {
			val = (T*)calloc(n,sizeof(T));
			this->n = n;
		} else if(this->n == n)  {
			return;
		} else {
			T* val = (T*)realloc(this->val, n*sizeof(T));
			if(val == NULL) 
				throw "Memory error";		// XXX This handling is bad but will probably never occur
			this->val = val;

			if(n > this->n)
				bzero(this->val+this->n, sizeof(T)*(n-this->n));
			this->n = n;
		}
	}
	/** Erase the array contents, i.e. set everything to zero */
	void clear() {
		bzero(this->val, sizeof(T)*this->n);
	}

	const T operator[](const size_t i) const { return this->val[i]; }
	T& operator[](const size_t i) { return this->val[i]; }
	/** Assign contents from another array to this one */
	Array<T>& operator=(const Array &src) {
		this->resize(src.n);	// Also assigns n
		memcpy(this->val, src.val, n*sizeof(T));
		return *this;
	}
	/** Assign a constant value to the array */
	Array<T>& operator=(const T &t) {
		for(size_t i=0;i< this->n; i++)
			this->val[i] = t;
		return *this;
	}

	T sum() const {
		if(this->n == 0 || this->val == NULL) return 0;
		T ret(0);
		for(size_t i=0;i<this->n;i++)
			ret += this->val[i];
		return ret;
	}
	T avg() const {
		if(this->n == 0 || this->val == NULL) return 0;
		return this->sum() / this->n;
	}
	T min() const {
		if(this->n == 0 || this->val == NULL) return 0;
		T ret = this->val[0];
		for(size_t i=1;i<this->n;i++)
			if(this->val[i] < ret) ret = this->val[i];
		return ret;
	}
	T max() const {
		if(this->n == 0 || this->val == NULL) return 0;
		T ret = this->val[0];
		for(size_t i=1;i<this->n;i++)
			if(this->val[i] > ret) ret = this->val[i];
		return ret;
	}
};

template <class T>
class Matrix : public Array<T> {
protected :
	size_t dims[2];
	size_t index(const size_t x, const size_t y) const { return dims[0]*y+x; }
public:
	/** Initialize a new empty matrix */
	Matrix() : Array<T>(0) {
		dims[0] = 0;
		dims[1] = 0;
	}
	/** Initialize a new (m x n) matrix */
	Matrix(const size_t m, const size_t n) : Array<T>(n*m) {
		dims[0] = m;
		dims[1] = n;
	}
	Matrix(const Matrix &src) {
		this->n = src.n;
		this->val = (T*)malloc(src.n*sizeof(T));
		memcpy(this->val, src.val, src.n*sizeof(T));
		this->dims[0] = src.dims[0];
		this->dims[1] = src.dims[1];
	}
	Matrix(Matrix &&src) {
		this->n = src.n;
		this->val = src.val;
		this->dims[0] = src.dims[0];
		this->dims[1] = src.dims[1];
		src.n = 0;
		src.val = NULL;
		src.dims[0] = 0;
		src.dims[1] = 0;
	}

	size_t size(const size_t i) const { return this->dims[i]; }
	size_t size() const { return Array<T>::size(); }
	
	
	/** Resize the matrix and clear it's contents */
	void resize(const size_t m, const size_t n) {
		Array<T>::resize(m*n);
		this->dims[0] = m;
		this->dims[1] = n;
		this->clear();
	}
	
	const T operator()(const size_t i, const size_t j) const { return this->val[index(i,j)]; }
	T& operator()(const size_t i, const size_t j) { return this->val[index(i,j)]; }

	/** Assign contents from another matrix to this one */
	Matrix<T>& operator=(const Matrix &src) {
		this->resize(src.n);	// Also assigns n
		memcpy(this->val, src.val, src.n*sizeof(T));
		this->dims[0] = src.dims[0];
		this->dims[1] = src.dims[1];
		return *this;
	}
	/** Assign a constant value to the array */
	Array<T>& operator=(const T &t) { return Array<T>::operator=(t); }
};

template <class T>
class Cube : public Array<T> {
protected :
	size_t dims[3];
	size_t index(const size_t x, const size_t y, const size_t z) const { return dims[0]*dims[1]*z+y*dims[0]+x; }
public:
	/** Initialize a new empty matrix */
	Cube() : Array<T>(0) {
		dims[0] = 0;
		dims[1] = 0;
		dims[2] = 0;
	}
	/** Initialize a new (m x n x o) cubus */
	Cube(const size_t m, const size_t n, const size_t o) : Array<T>(m*n*o) {
		dims[0] = m;
		dims[1] = n;
		dims[2] = o;
	}
	Cube(const Cube &src) {
		this->n = src.n;
		this->val = (T*)malloc(src.n*sizeof(T));
		memcpy(this->val, src.val, src.n*sizeof(T));
		this->dims[0] = src.dims[0];
		this->dims[1] = src.dims[1];
		this->dims[2] = src.dims[2];
	}
	Cube(Cube &&src) {
		this->n = src.n;
		this->val = src.val;
		this->dims[0] = src.dims[0];
		this->dims[1] = src.dims[1];
		this->dims[2] = src.dims[2];
		src.n = 0;
		src.val = NULL;
		src.dims[0] = 0;
		src.dims[1] = 0;
		src.dims[2] = 0;
	}

	size_t size(const size_t i) const { return this->dims[i]; }
	size_t size() const { return Array<T>::size(); }

	/** Resize the cube and clear it's contents */
	void resize(const size_t n1, const size_t n2, const size_t n3) {
		Array<T>::resize(n1*n2*n3);
		this->dims[0] = n1;
		this->dims[1] = n2;
		this->dims[2] = n3;
		this->clear();
	}
	
	const T operator()(const size_t i, const size_t j, const size_t k) const { return this->val[index(i,j,k)]; }
	T& operator()(const size_t i, const size_t j, const size_t k) { return this->val[index(i,j,k)]; }

	/** Assign contents from another matrix to this one */
	Cube<T>& operator=(const Cube<T> &src) {
		this->resize(src.n);	// Also assigns n
		memcpy(this->val, src.val, src.n*sizeof(T));
		this->dims[0] = src.dims[0];
		this->dims[1] = src.dims[1];
		this->dims[2] = src.dims[2];
		return *this;
	}
	/** Assign a constant value to the array */
	Array<T>& operator=(const T &t) { return Array<T>::operator=(t); }
};

template <class T>
class Tesseract : public Array<T> {
protected :
	size_t dims[4];
	size_t index(const size_t x1, const size_t x2, const size_t x3, const size_t x4) const {
		return x1+x2*dims[0]+x3*dims[0]*dims[1]+x4*dims[0]*dims[1]*dims[2];
	}
public:
	/** Initialize a new empty matrix */
	Tesseract() : Array<T>(0) {
		for(int i=0;i<4;i++)
			dims[i] = 0;
	}
	/** Initialize a new (m x n x o) cubus */
	Tesseract(const size_t n1, const size_t n2, const size_t n3, const size_t n4) : Array<T>(n1*n2*n3*n4) {
		dims[0] = n1;
		dims[1] = n2;
		dims[2] = n3;
		dims[3] = n4;
	}
	Tesseract(const Tesseract &src) {
		this->n = src.n;
		this->val = (T*)malloc(src.n*sizeof(T));
		memcpy(this->val, src.val, src.n*sizeof(T));
		for(int i=0;i<4;i++)
			this->dims[i] = src.dims[i];
	}
	Tesseract(Tesseract &&src) {
		this->n = src.n;
		this->val = src.val;
		for(int i=0;i<4;i++) {
			this->dims[i] = src.dims[i];
			src.dims[i] = 0;
		}
		src.n = 0;
		src.val = NULL;
	}

	size_t size(const size_t i) const { return this->dims[i]; }
	size_t size() const { return Array<T>::size(); }

	/** Resize the tesseract and clear it's contents */
	void resize(const size_t n1, const size_t n2, const size_t n3, const size_t n4) {
		Array<T>::resize(n1*n2*n3*n4);
		this->dims[0] = n1;
		this->dims[1] = n2;
		this->dims[2] = n3;
		this->dims[3] = n4;
		this->clear();
	}
	
	const T operator()(const size_t x1, const size_t x2, const size_t x3, const size_t x4) const { return this->val[index(x1,x2,x3,x4)]; }
	T& operator()(const size_t x1, const size_t x2, const size_t x3, const size_t x4) { return this->val[index(x1,x2,x3,x4)]; }

	/** Assign contents from another matrix to this one */
	Tesseract<T>& operator=(const Tesseract<T> &src) {
		this->resize(src.n);	// Also assigns n
		memcpy(this->val, src.val, src.n*sizeof(T));
		for(int i=0;i<4;i++)
			this->dims[i] = src.dims[i];
		return *this;
	}
	/** Assign a constant value to the array */
	Array<T>& operator=(const T &t) { return Array<T>::operator=(t); }
};

}

#endif
