/* =============================================================================
 *
 * Title:       Easy access to HDF5 files
 * Author:      Felix Niederwanger
 * License:     MIT (http://opensource.org/licenses/MIT)
 * Description: Library, code file
 * =============================================================================
 */

#include "hdf5.hpp"

#include <cstring>

#include <unistd.h>
#include <hdf5.h>

static inline bool hdf5_file_exists(const char* pathname) {
	return ::access( pathname, F_OK ) == 0;
}



using namespace std;
using namespace numeric;

namespace hdf5 {

static string extractFilename(string pathname) {
	size_t index = pathname.rfind('/');
	if (index == string::npos) return pathname;
	return pathname.substr(index+1);
}


void HDF5Exception::clearErrorStack() {
	H5Eclear(H5E_DEFAULT);
}

void HDF5Exception::printStack(FILE* stream) {
	H5Eprint(H5E_DEFAULT, stream);
}


HDF5File::HDF5File(HDF5File &file) {
	this->fid = 0;
	this->_rootGroup = NULL;
	this->init(file._filename.c_str());
}
HDF5File::HDF5File(std::string filename, bool readOnly) {
	this->fid = 0;
	this->_rootGroup = NULL;
	this->init(filename.c_str(), readOnly);
}
HDF5File::HDF5File(const char* filename, bool readOnly) {
	this->fid = 0;
	this->_rootGroup = NULL;
	this->init(filename, readOnly);
}

void HDF5File::init(const char* filename, bool readOnly) {
	if(strlen(filename) == 0) throw HDF5Exception("Empty filename");
	this->_filename = filename;


	if(hdf5_file_exists(filename)) {
		int flags;
		if(readOnly)
			flags = H5F_ACC_RDONLY;
		else
			flags = H5F_ACC_RDWR;
		this->fid = H5Fopen(filename, flags, H5P_DEFAULT);
	} else
		this->fid = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if(this->fid < 0) throw HDF5Exception("Error opening HDF5 file");

	// Immediately open root group
	this->_rootGroup = new HDF5Group(this, "/");
	// Note: Objects are now added via the HDF5Object constructor
}

HDF5File::~HDF5File() {
	this->close();
}

void HDF5File::close(void) {
	// Close all opened HDF5 objects
	vector<HDF5Object*> objects(this->_objects);	// Copy object list, since deleted object will manipulate the list
	for(vector<HDF5Object*>::iterator it = objects.begin(); it!= objects.end(); ++it) {
		delete *it;
	}
	this->_objects.clear();
	this->_rootGroup = NULL;		// Already deleted within the object iterator

	// Ultimately, close file
	if(this->fid > 0) H5Fclose(this->fid);
	this->fid = 0;
}

string HDF5File::filename() {
	string result = this->_filename;
	const size_t index = result.rfind('/');
	if(index != string::npos)
		result = result.substr(index+1);
	return result;
}

string HDF5File::pathname() {
	return this->_filename;
}

void HDF5File::removeObject(HDF5Object *obj) {
	if(obj == NULL) return;
	if(obj == this->_rootGroup) return;		// Root group cannot be deleted
	for(vector<HDF5Object*>::iterator it = this->_objects.begin(); it!= this->_objects.end(); ++it) {
		if( (*it) == obj) {
			this->_objects.erase(it);
			return;
		}
	}
}

void HDF5File::addObject(HDF5Object *obj) {
	if(obj == NULL) return;
	else this->_objects.push_back(obj);
}

HDF5Group* HDF5File::group(std::string name) {
	HDF5Group *group = new HDF5Group(this, name);
	// Note: Objects are now added via the HDF5Object constructor
	//this->_objects.append(group);
	return group;
}

HDF5Group* HDF5File::rootGroup() {
	return this->_rootGroup;
}

HDF5Dataset* HDF5File::dataset(std::string name) {
	HDF5Dataset *ds = new HDF5Dataset(this, name);
	// Note: Objects are now added via the HDF5Object constructor
	// this->_objects.append(ds);
	return ds;
}

HDF5Group* HDF5File::createGroup(std::string name) {
	if (name.length() == 0) throw HDF5Exception("Empty group name");
	if(name.at(0) != '/') name = "/" + name;

	hid_t gid;		// Group id
	gid = H5Gcreate(this->fid, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if(gid < 0) throw HDF5Exception("Error creating group");
	if( H5Gclose(gid) < 0) throw HDF5Exception("Error closing group after creation");
	return this->group(name);
}

HDF5Dataset* HDF5File::createDataset(std::string name, int nDims, size_t* dimSize, int flags) {
	if (name.length() == 0) throw HDF5Exception("Empty dataset name");

	// Check for absolute path, and make it a child of root, if it is a name only
	if(name.at(0) != '/') name = "/" + name;
	(void)flags;		// Not used at this moment

	// Identifiers
	hid_t    dataset_id = 0;
	hid_t    dataspace_id = 0;
	hsize_t* dims = new hsize_t[nDims];
	// herr_t   status;
	try {
		hid_t dtype_id = H5T_NATIVE_DOUBLE;
		// XXX: Include flags for the following types:
		// 		H5T_NATIVE_FLOAT
		//		H5T_NATIVE_INT
		//		H5T_NATIVE_LONG
		// see https://www.hdfgroup.org/HDF5/Tutor/datatypes.html


		/* Create the data space for the dataset. */
		for(int i=0;i<nDims;i++)
			dims[i] = dimSize[i];
		dataspace_id = H5Screate_simple(nDims, dims, NULL);
		if(dataspace_id < 0) throw HDF5Exception("Error creating dataspace");

		/* Create the dataset. */
		dataset_id = H5Dcreate2(this->fid, name.c_str(), dtype_id, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
		if(dataset_id < 0) throw HDF5Exception("Error creating dataset");


		// Cleanup
		delete[] dims;
		if(dataset_id > 0)   H5Dclose(dataset_id);
		if(dataspace_id > 0) H5Sclose(dataspace_id);


		// Open dataset
		return this->dataset(name);
	} catch (...) {
		// Cleanup
		delete[] dims;

		// Close in reverse order
		if(dataset_id > 0)   H5Dclose(dataset_id);
		if(dataspace_id > 0) H5Sclose(dataspace_id);
		throw;
	}
}





HDF5Object::HDF5Object() {
	this->_file = NULL;
	this->_id = 0;
	this->_type = 0;
}

HDF5Object::HDF5Object(HDF5File *file) {
	this->_file = file;
	this->_id = 0;
	this->_type = 0;
	if(this->_file != NULL)
		this->_file->addObject(this);
}

HDF5Object::~HDF5Object(void) {
	this->close();
	if(this->_file != NULL)
		this->_file->removeObject(this);
}

bool HDF5Object::isClosed(void) {
	return this->_id <= 0;
}

bool HDF5Object::isOpened(void) {
	return this->_id > 0;
}

struct {
	std::vector<string> *list;
	int type;
} typedef _hdf5_iterator_op_;


// hdf5 iterator function
herr_t _hdf5_iteration_func (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data) {
	(void)(info);		// Mark used
	_hdf5_iterator_op_ *op = (_hdf5_iterator_op_*)operator_data;
	std::vector<string> *list = op->list;

	herr_t          status;
	H5O_info_t      infobuf;
	status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
	if(status < 0) {
		// Error handling
		return -1;
	}
	const H5O_type_t type = infobuf.type;

	// Check type
	switch(type) {
	case H5O_TYPE_GROUP:
		if((op->type & HDF5Object::TYPE_GROUP) == 0) return 0;
		break;
	case H5O_TYPE_DATASET:
		if((op->type & HDF5Object::TYPE_DATASET) == 0) return 0;
		break;
	case H5O_TYPE_NAMED_DATATYPE:
		if((op->type & HDF5Object::TYPE_ATTRIBUTE) == 0) return 0;
		break;
	default:
		// This case should never trigger.
		return 0;
	}

	list->push_back(string(name));
	return 0;
}

std::vector<string> HDF5Object::getItemNames(void) {
	return this->getItemNames(HDF5Object::TYPE_ALL);
}

std::vector<string> HDF5Object::getItemNames(const int type) {
	std::vector<string> result;
	if(isClosed()) return result;

	_hdf5_iterator_op_ op;
	herr_t status;
	op.list = &result;
	op.type = type;
	status = H5Literate(this->_id,
			H5_INDEX_NAME,
			H5_ITER_NATIVE,             // Fastest possible iteration
			NULL,
			_hdf5_iteration_func,
			(void*)&op);
	if(status != 0)
		throw HDF5Exception("Error iterating in HDF5 object");
	return result;
}

std::vector<string> HDF5Object::getSubGroups(void) {
	return getItemNames(HDF5Object::TYPE_GROUP);
}

std::vector<string> HDF5Object::getSubDatasets(void) {
	return getItemNames(HDF5Object::TYPE_DATASET);
}


HDF5Group* HDF5Object::openGroup(string pathname) {
	if(pathname.length() == 0) return NULL;

	// Check for absolute pathname
	if(pathname.at(0) != '/')
		pathname = this->groupPathname() + pathname;

	return this->_file->group(pathname);
}

HDF5Dataset* HDF5Object::openDataset(string pathname) {
	if(pathname.length() == 0) return NULL;

	// Check for absolute pathname
	if(pathname.at(0) != '/')
		pathname = this->groupPathname() + pathname;

	return this->_file->dataset(pathname);
}

string HDF5Object::pathname(void) { return this->_pathname; }

string HDF5Object::groupPathname(void) {
	string result;
	if(this->isGroup()) {
		result = this->_pathname;
	} else {
		const size_t index = this->_pathname.rfind('/');
		if(index == string::npos) return "/";
		else
			result = this->_pathname.substr(0, index-1);
	}
	if(result.at(result.length()-1) != '/') result += "/";
	return result;
}

void HDF5Object::linkDelete(const char* name) {
	if(this->_id == 0) throw HDF5Exception("Object already closed");
	if(this->_id < 0) throw HDF5Exception("Negative object identifier");

	hid_t lapl_id = 0;
	const herr_t status = H5Ldelete( this->_id, name, lapl_id );
	if(status < 0) throw HDF5Exception("Error deleting link");
}

int HDF5Object::type() { return this->_type; }

bool HDF5Object::isGroup(void) { return (this->type() & HDF5Object::TYPE_GROUP) != 0; }
bool HDF5Object::isDataset(void)  { return (this->type() & HDF5Object::TYPE_DATASET) != 0; }
bool HDF5Object::isAttribute(void)  { return (this->type() & HDF5Object::TYPE_ATTRIBUTE) != 0; }

/**
 * Write an attribute to the given identifier
 * @param id Identifier where the attribute is written to
 */
static void writeAttributeArray(hid_t id, const char* name, hid_t mem_type, void* values, int rank, size_t* dims) {
	hid_t attr_id;
	herr_t ret;
	hid_t  dspace;

	// Create dataspace
	dspace = H5Screate(H5S_SIMPLE);
	if(dspace < 0) throw HDF5Exception("Error creating dataspace");
	hsize_t *h_dims = new hsize_t[rank];
	for(int i=0;i<rank;i++)
		h_dims[i] = (hsize_t)dims[i];
	ret  = H5Sset_extent_simple(dspace, rank, h_dims, NULL);
	delete[] h_dims;
	if(ret < 0) {
		H5Sclose(dspace);
		throw HDF5Exception("Error setting up dataspace");
	}

	// Create array
	attr_id = H5Acreate2 (id, name, mem_type, dspace, H5P_DEFAULT, H5P_DEFAULT);
	if(attr_id < 0) {
		H5Sclose(dspace);
		throw HDF5Exception("Error creating attribute");
	}

	// Write array
	ret = H5Awrite(attr_id, mem_type, values);
	H5Sclose(dspace);
	H5Aclose(attr_id);
	if(ret < 0)
		throw HDF5Exception("Error writing attribute");
}


/**
 * Write an attribute to the given identifier
 * @param id Identifier where the attribute is written to
 */
static void writeAttribute(hid_t id, const char* name, hid_t mem_type, void* value) {
	const size_t dims[1] = {1};
	writeAttributeArray(id, name, mem_type, value, 1, (size_t*)dims);
#if 0
	hid_t attr_id;
	herr_t ret;
	hid_t  dspace;

	dspace = H5Screate(H5S_SCALAR);

	if(dspace < 0) throw HDF5Exception("Error creating dataspace");
	ret  = H5Sset_extent_simple(dspace, 1, dims, NULL);
	if(ret < 0) {
		H5Sclose(dspace);
		throw HDF5Exception("Error setting up dataspace");
	}
	attr_id = H5Acreate2 (id, name, mem_type, dspace, H5P_DEFAULT, H5P_DEFAULT);
	if(attr_id < 0) {
		H5Sclose(dspace);
		throw HDF5Exception("Error creating attribute");
	}
	ret = H5Awrite(attr_id,mem_type, value);
	H5Sclose(dspace);
	H5Aclose(attr_id);
	if(ret < 0)
		throw HDF5Exception("Error writing attribute");
#endif
}




HDF5Group::HDF5Group(HDF5File *file, string name) : HDF5Object(file) {
	this->_pathname = name;
	this->_id = H5Gopen(this->fid(), name.c_str(), H5P_DEFAULT);
	if(this->_id < 0) throw HDF5Exception("Error opening group");
	this->attrs = HDF5AttributeManager(this);

}

void HDF5Group::close(void) {
	if(this->_id > 0)
		H5Gclose(this->_id);
	this->_id = 0;
}

string HDF5Group::name(void) {
	return extractFilename(this->pathname());
}

string HDF5Group::relativePath(string name) {
	string pathname = this->pathname();
	if(pathname.length() > 0 && pathname.at(pathname.length()-1) != '/') pathname += '/';
	if(name.length() == 0) return pathname;

	if(name.at(0) == '/') {			// Absolute pathname?
		return name;
	} else {
		pathname += name;
		return pathname;
	}
}

HDF5Dataset* HDF5Group::dataset(string name) { return this->_file->dataset(relativePath(name)); }
HDF5Group* HDF5Group::group(string name) { return this->_file->group(relativePath(name)); }


HDF5Group* HDF5Group::createGroup(std::string name) {
	// Create pathname
	if (name.length() == 0) throw HDF5Exception("Empty group name");
	string pathname = name;
	if(pathname.at(0) != '/') pathname = this->groupPathname() + name;
	return this->_file->createGroup(pathname);
}

HDF5Dataset* HDF5Group::createDataset(std::string name, int nDims, size_t* dims, int flags) {
	string pathname = string(name);
	if(pathname.length() == 0) throw HDF5Exception("Empty dataset pathname");

	// Check for absolute path
	if(pathname.at(0) != '/') {
		pathname = string(this->_pathname);
		if(pathname.at(pathname.length()-1) != '/') pathname += '/';
		pathname += name;
	}
	return this->_file->createDataset(pathname, nDims, dims, flags);
}












HDF5Dataset::HDF5Dataset(HDF5File *file, string pathname) : HDF5Object(file) {
	if(pathname.length() == 0) throw HDF5Exception("Cannot open empty pathname");
	this->_pathname = pathname;
	this->d_dims = NULL;
	this->attrs = HDF5AttributeManager(this);

	this->_id = H5Dopen(this->fid(), pathname.c_str(), H5P_DEFAULT);
	if(this->_id < 0) throw HDF5Exception("Error opening dataset");

	// Open dataspace and get properties
	const hid_t dataspace = H5Dget_space(this->_id);
	if(dataspace < 0) throw HDF5Exception("Error getting dataspace from dataset");

	try {
		herr_t status = 0;

		this->d_datatype    = H5Dget_type(this->_id);
		this->d_class       = H5Tget_class(d_datatype);
		this->d_order       = H5Tget_order(d_datatype);
		this->d_size        = H5Tget_size(d_datatype);


		// Get rank and dimensions
		this->d_rank        = H5Sget_simple_extent_ndims(dataspace);
		this->d_dims        = new hsize_t[d_rank];
		status              = H5Sget_simple_extent_dims(dataspace, d_dims, NULL);

		if ((status < 0) || ( (int)status != (int)d_rank))
			throw HDF5Exception("Error getting dataset properties");


		H5Sclose(dataspace);
	} catch(...) {
		// Emergency close
		H5Sclose(dataspace);
		H5Dclose(this->_id);
		throw;
	}

}

HDF5Dataset::~HDF5Dataset() {
	this->close();

	if(this->d_dims != NULL)
		delete[] this->d_dims;
}


long HDF5Dataset::getStorageSize(void) {
	if(this->_id < 0) return -1L;
	hsize_t storage = H5Dget_storage_size( this->_id ) ;
	return (long)storage;
}

void HDF5Dataset::close(void) {
	if(this->_id > 0) H5Dclose(this->_id);
	this->_id = 0;
}

string HDF5Dataset::name(void) {
	return extractFilename(this->pathname());
}


bool HDF5Dataset::isInteger() {
	return d_class == H5T_INTEGER;
}

bool HDF5Dataset::isFloat() {
	return d_class == H5T_FLOAT;
}

bool HDF5Dataset::isLittleEndian() {
	return d_order == H5T_ORDER_LE;
}

size_t HDF5Dataset::typeSize(void) {
	return this->d_size;
}

size_t HDF5Dataset::dims(void) {
	return this->d_rank;
}

size_t HDF5Dataset::cells(void) {
	size_t result = 1;
	for(int i=0;i<d_rank;i++)
		result *= (size_t)(this->d_dims[i]);
	return result;
}

size_t HDF5Dataset::size(void) {
	return this->cells() * this->typeSize();
}

size_t HDF5Dataset::dims(int dim) {
	return (size_t)this->d_dims[dim];
}


static size_t hdf5_read(hid_t dataset, double *dst, const size_t dims, const size_t* n, const size_t* offset_ = NULL) {
	herr_t      status = 0;
	hid_t       memspace = 0;
	hid_t       dataspace = 0;

	/* Hyperslab in the file */
	hsize_t* count = new hsize_t[dims];
	/* Hyperslab offset in the file */
	hsize_t* offset = new hsize_t[dims];
	/* Hyperslab offset in memory */
	hsize_t* offset_out = new hsize_t[dims];
	/* Memory space dimensions */
	hsize_t* dimsm = new hsize_t[dims];

	for(size_t i = 0;i<dims;i++) {
		count[i] = n[i];
		if(offset_ == NULL)
			offset[i] = 0;
		else
			offset[i] = offset_[i];
		offset_out[i] = 0;
	}

	ssize_t result = -1;
	try {
		dataspace = H5Dget_space(dataset);
		if(dataspace < 0) throw HDF5Exception("Error getting dataspace");

		if(offset_ != NULL) {
			status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);
			if(status < 0) throw HDF5Exception("Error selecting hyperslab");
		}

		// Define memory space
		const hsize_t n_dims = hsize_t(dims);
		for(size_t i = 0;i<dims;i++) {
			dimsm[i] = count[i];
			result *= count[i];
		}
		memspace = H5Screate_simple(n_dims, dimsm, NULL);
		if(memspace < 0) throw HDF5Exception("Error creating memspace");

		if(offset_ != NULL) {
			status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL, count, NULL);
			if(status < 0) throw HDF5Exception("Error selecting memory space hyperslab");
		}

		// Read from file
		status = H5Dread(dataset, H5T_NATIVE_DOUBLE, memspace, dataspace, H5P_DEFAULT, dst);
		if(status < 0) throw HDF5Exception("Error reading from HDF5 file");

		// Everything went fine
		result = -result;
	} catch (...) {
		delete[] count;
		delete[] offset;
		delete[] offset_out;
		delete[] dimsm;
		// Cleanup
		if(dataspace > 0) H5Sclose(dataspace);
		if(memspace > 0)  H5Sclose(memspace);
		throw;
	}

	// Close and cleanup
	delete[] count;
	delete[] offset;
	delete[] offset_out;
	delete[] dimsm;
	H5Sclose(dataspace);
	H5Sclose(memspace);


	return (size_t)result;
}


// Write array dst to the given dataset hid_t
// Memory -> File
static size_t hdf5_write(hid_t dataset, double *dst, const size_t dims, const size_t* n, const size_t* offset_ = NULL) {
	herr_t      status = 0;
	hid_t       memspace = 0;
	hid_t       dataspace = 0;

	/* Hyperslab in the file */
	hsize_t* count = new hsize_t[dims];
	/* Hyperslab offset in the file */
	hsize_t* offset = new hsize_t[dims];
	/* Hyperslab offset in memory */
	hsize_t* offset_out = new hsize_t[dims];
	/* Memory space dimensions */
	hsize_t* dimsm = new hsize_t[dims];

	for(size_t i = 0;i<dims;i++) {
		count[i] = n[i];
		if(offset_ == NULL)
			offset[i] = 0;
		else
			offset[i] = offset_[i];
		offset_out[i] = 0;
	}

	ssize_t result = -1;
	try {
		dataspace = H5Dget_space(dataset);
		if(dataspace < 0) throw HDF5Exception("Error getting dataspace");

		if(offset_ != NULL) {
			status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);
			if(status < 0) throw HDF5Exception("Error selecting hyperslab");
		}

		// Define memory space
		const hsize_t n_dims = hsize_t(dims);
		for(size_t i = 0;i<dims;i++) {
			dimsm[i] = count[i];
			result *= count[i];
		}
		memspace = H5Screate_simple(n_dims, dimsm, dimsm);
		if(memspace < 0) throw HDF5Exception("Error creating memspace");
		if(offset_ != NULL) {
			status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL, count, NULL);
			if(status < 0) throw HDF5Exception("Error selecting memory space hyperslab");
		}

		// Write to file
		status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, memspace, dataspace, H5P_DEFAULT, dst);
		if(status < 0) throw HDF5Exception("Error reading from HDF5 file");

		// Everything went fine
		result = -result;
	} catch (...) {
		delete[] count;
		delete[] offset;
		delete[] offset_out;
		delete[] dimsm;
		// Cleanup
		if(dataspace > 0) H5Sclose(dataspace);
		if(memspace > 0)  H5Sclose(memspace);
		throw;
	}

	// Close and cleanup
	delete[] count;
	delete[] offset;
	delete[] offset_out;
	delete[] dimsm;
	H5Sclose(dataspace);
	H5Sclose(memspace);


	return (size_t)result;
}



double HDF5Dataset::read_2d(size_t x, size_t y) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");

	double buf;
	size_t n[2] = {1,1};
	// Remember: x,y are swapped
	size_t offset[2] = {y,x};
	hdf5_read(this->_id, &buf, 2, n, offset);
	return buf;
}

double HDF5Dataset::operator()(size_t x, size_t y) {
	return this->read_2d(x,y);
}

size_t HDF5Dataset::read_1d(double* buf, const size_t n) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");
	size_t dims[] = { n };
	return hdf5_read(this->_id, buf, 1, dims);
}

size_t HDF5Dataset::read(double *buf, const size_t n, const size_t* dims) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");
	return hdf5_read(this->_id, buf, n, dims);
}

size_t HDF5Dataset::read(double** array) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");

	const size_t size = this->dims(0);
	double* buf = new double[size];
	read_1d(buf, size);
	*array = buf;
	return size;
}

size_t HDF5Dataset::read(double*& array) {
	return this->read(&array);
}

size_t HDF5Dataset::write(double* array, size_t n) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");
	size_t dims[] = { n };
	return hdf5_write(this->_id, array, 1, dims);
}

#ifdef _FLEXLIB_ARRAY_HPP

Array1d<double>* HDF5Dataset::read_1d(void) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");

	double* buf;
	const size_t size = this->read(buf);

	Array1d<double>* result = new Array1d<double>(size);
	for(size_t i=0;i<size;i++)
		result->set(i, buf[i]);
	delete[] buf;
	return result;
}

#endif

HDF5Attribute::HDF5Attribute() : HDF5Object() {
	this->_name = "";
	this->_parent = NULL;
	this->_id = 0;
	this->manager = NULL;
}

HDF5Attribute::HDF5Attribute(const HDF5Attribute &attr) : HDF5Object(attr._file) {
	this->_name = attr._name;
	this->_parent = attr._parent;
	this->_id = 0;
	this->manager = attr.manager;
	this->open();
}

HDF5Attribute::HDF5Attribute(const HDF5AttributeManager* manager, std::string name) : HDF5Object(manager->parent->_file) {
	this->_name = name;
	this->_parent = manager->parent;
	this->_id = 0;
	this->manager = (HDF5AttributeManager*)manager;
	this->open();

}
HDF5Attribute::~HDF5Attribute() {
	this->close();
}

void HDF5Attribute::open(void) {
	if(this->_id > 0 || this->_parent == NULL) return;
	this->_id = H5Aopen(this->_parent->_id, this->_name.c_str(), H5P_DEFAULT);
	if(this->_id < 0) throw HDF5Exception("Error opening attribute");
}

void HDF5Attribute::close(void) {
	if(this->_id > 0) {
		if (H5Aclose(this->_id) < 0) throw HDF5Exception("Error closing attribute");
	}
	this->_id = 0;
}


const string HDF5Attribute::name(void) {
	return this->_name;
}


int HDF5Attribute::readInt(void) {
	this->open();
	int result = 0;
	H5Aread(this->_id, H5T_NATIVE_INT32, &result);
	return result;
}

float HDF5Attribute::readFloat(void) {
	this->open();
	float result = 0.0F;
	H5Aread(this->_id, H5T_NATIVE_FLOAT, &result);
	return result;
}

long HDF5Attribute::readLong(void) {
	this->open();
	long result = 0L;
	H5Aread(this->_id, H5T_NATIVE_LONG, &result);
	return result;
}

double HDF5Attribute::readDouble(void) {
	this->open();
	double result = 0.0;
	H5Aread(this->_id, H5T_NATIVE_DOUBLE, &result);
	return result;
}



Cube<double> HDF5Dataset::readCube() {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");
	if(this->d_rank != 3) throw HDF5Exception("Cannot read cube from not-2d dataset");
	size_t dims[3] = {this->d_dims[0], this->d_dims[1], this->d_dims[2]};
	const size_t size = dims[0] * dims[1] * dims[2];
	Cube<double> result(dims[0], dims[1], dims[2]);

	double *buf = new double[size];
	this->read(buf, 3, dims);

	size_t i=0;
	for(size_t ix=0; ix < dims[0]; ix++) {
		for(size_t iy=0; iy < dims[1]; iy++) {
			for(size_t iz=0; iz < dims[2]; iz++) {
				const double value = buf[i++];
				result(ix,iy,iz) = value;
			}
		}
	}
	delete[] buf;

	return result;
}

void HDF5Dataset::writeCube(const Cube<double> &cube) {
	if(this->isClosed()) throw HDF5Exception("Dataset closed");
	size_t dims[3] = { cube.size(0), cube.size(1), cube.size(2) };
	const size_t size = cube.size();

	// Build buffer
	double *buf = new double[size];
	size_t i=0;

	for(size_t ix=0; ix < dims[0]; ix++) {
		for(size_t iy=0; iy < dims[1]; iy++) {
			for(size_t iz=0; iz < dims[2]; iz++) {
				buf[i++] = cube(ix,iy,iz);
			}
		}
	}

	// Write to actual HDF5
	try {
		hdf5_write(this->_id, buf, 3, dims);
	} catch (...) {
		delete[] buf;
		throw;
	}
	delete[] buf;
}

void HDF5Dataset::writeArray(valarray<double> &array) {
	// Must copy to local buffer
	const size_t size = array.size();
	double* buf = new double[size];
	for(size_t i=0;i<size;i++) buf[i] = array[i];

	size_t dims[1] = {size};
	try {
		hdf5_write(this->_id, buf, 1, dims);
	} catch (...) {
		delete[] buf;
		throw;
	}
	delete[] buf;
}



HDF5AttributeManager::HDF5AttributeManager() {
	this->parent = NULL;
}

HDF5AttributeManager::HDF5AttributeManager(HDF5Object*parent) {
	this->parent = parent;
}

HDF5AttributeManager::~HDF5AttributeManager() {
	// Close attribute manager
}


/* Operator function for iterating over attributes and writing all names into a flex::string vector */
static herr_t attr_iterator_names(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata) {
	std::vector<string>* result = (std::vector<string>*)opdata;
	(void) ainfo;
	(void) loc_id;

	string attrName(name);
	result->push_back(attrName);
	return 0;
}

std::vector<HDF5Attribute> HDF5AttributeManager::attributes(void) {
	std::vector<string> names = this->names();
	std::vector<HDF5Attribute> result;

	// Create attribute instances
	for(std::vector<string>::iterator it = names.begin(); it != names.end(); ++it) {
		string name = *it;
		HDF5Attribute attr(this, name);		// Throws HDF5Exception if opening fails
		result.push_back(attr);
	}

	return result;
}

vector<string> HDF5AttributeManager::names(void) {
	std::vector<string> names;		// Names of the attributes

	herr_t  ret;                /* Return value */
	// Iterate over all attributes and fetch their names
	ret = H5Aiterate2(this->parent->_id, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_iterator_names, &names);
	if(ret != 0)
		throw HDF5Exception("Error iterating over attributes");

	return names;
}



bool HDF5AttributeManager::hasAttribute(const char* name) {
	return this->hasAttribute(string(name));
}

bool HDF5AttributeManager::hasAttribute(const std::string name) {
	if(name.length() == 0) return false;

	// Search in names for attribute
	std::vector<string> names = this->names();
	for(vector<string>::iterator it=names.begin(); it!=names.end(); ++it)
		if (name == *it) return true;
	return false;
}



HDF5Attribute HDF5AttributeManager::attribute(const char* name) {
	HDF5Attribute attr(this, name);
	return attr;
}

HDF5Attribute HDF5AttributeManager::attribute(string name) {
	HDF5Attribute attr(this, name);
	return attr;
}

HDF5Attribute HDF5AttributeManager::operator[](std::string name) {
	HDF5Attribute attr(this, name);
	return attr;
}

HDF5Attribute HDF5AttributeManager::operator[](const char* name) {
	HDF5Attribute attr(this, name);
	return attr;
}



void HDF5AttributeManager::create(const std::string name, const int value) {
	int v = value;
	writeAttribute(this->parent->_id, name.c_str(), H5T_NATIVE_INT32, &v);
}

void HDF5AttributeManager::create(const std::string name, const long value) {
	long v = value;
	writeAttribute(this->parent->_id, name.c_str(), H5T_NATIVE_LONG, &v);
}

void HDF5AttributeManager::create(const std::string name, const float value) {
	float v = value;
	writeAttribute(this->parent->_id, name.c_str(), H5T_NATIVE_FLOAT, &v);
}

void HDF5AttributeManager::create(const std::string name, const double value) {
	double v = value;
	writeAttribute(this->parent->_id, name.c_str(), H5T_NATIVE_DOUBLE, &v);
}

void HDF5AttributeManager::createArray(const std::string name, const int* array, const size_t len) {
	const size_t dims[1] = {len};
	writeAttributeArray(this->parent->_id, name.c_str(), H5T_NATIVE_INT32, (void*)array, 1, (size_t*)dims);
}

void HDF5AttributeManager::createArray(const std::string name, const double* array, const size_t len) {
	const size_t dims[1] = {len};
	writeAttributeArray(this->parent->_id, name.c_str(), H5T_NATIVE_DOUBLE, (void*)array, 1, (size_t*)dims);
}


void HDF5AttributeManager::create(std::string name, const char* value) {
	const size_t len = strlen(value);
	this->create(name, value, len);
}

void HDF5AttributeManager::create(const std::string name, const char* str, const size_t len) {
	hid_t ds_id;
	hid_t ret;
	hid_t atype;
	hsize_t dimsa[1] = {1};
	hid_t attr;

	ds_id  = H5Screate_simple(1, dimsa, NULL);
	atype = H5Tcopy(H5T_C_S1);
	ret = H5Tset_size(atype, len);
	ret = H5Tset_strpad(atype, H5T_STR_NULLTERM);
	attr = H5Acreate(this->parent->_id, name.c_str(), atype, ds_id, H5P_DEFAULT, H5P_DEFAULT);
	if(attr < 0)
		throw HDF5Exception("Error creating attribute");

	ret = H5Awrite(attr, atype, str);
	if(ret < 0)
		throw HDF5Exception("Error writing string to attribute");

	ret |= H5Sclose(ds_id);
	ret |= H5Aclose(attr);

	if(ret < 0)
		throw HDF5Exception("Error closing attribute");
}

void HDF5AttributeManager::create(const std::string name, const std::string value) {
	this->create(name, value.c_str(), value.length());
}

int HDF5AttributeManager::readInt(const std::string name, bool *ok) {
	return (int)this->readDouble(name,ok);
}

long HDF5AttributeManager::readLong(const std::string name, bool *ok) {
	return (long)this->readDouble(name,ok);
}

float HDF5AttributeManager::readFloat(const std::string name, bool *ok) {
	return (float)this->readDouble(name,ok);
}

double HDF5AttributeManager::readDouble(const std::string name, bool *ok) {
	hid_t id;
	double result = 0.0;

	id = H5Aopen(this->parent->_id, name.c_str(), H5P_DEFAULT);
	if(id < 0) goto error;
	H5Aread(id, H5T_NATIVE_DOUBLE, &result);
	if(H5Aclose(id) < 0) {
		// Close failed ... Swallow here
	}
	if(ok!=NULL) *ok = true;
	return result;

	error:
	if(id > 0) H5Aclose(id);
	if(ok!=NULL) *ok = false;
	return -1;

}

std::string HDF5AttributeManager::readString(const std::string name, bool *ok) {
	hid_t id = 0;
	hid_t atype = 0;
	hid_t aspace = 0;
	herr_t stat = 0;
	size_t size = 0;
	hsize_t sdim[128];
	int rank = 0;
	char* buf = NULL;
	size_t totalSize = 0;
	string str;

	id = 0;
	id = H5Aopen(this->parent->_id, name.c_str(), H5P_DEFAULT);
	if(id < 0)
		goto error;

	atype  = H5Aget_type(id);
	if(atype < 0) goto error;
	aspace = H5Aget_space(id);
	if(aspace < 0) goto error;
	size = H5Tget_size (atype);
	rank = H5Sget_simple_extent_ndims(aspace);
	if(rank < 0 || rank >= 128) goto error;
	if(H5Sget_simple_extent_dims(aspace, sdim, NULL) < 0) goto error;

	totalSize = size;
	for(int i=0;i<rank;i++) totalSize *= sdim[i];

	buf = new char[totalSize];
	stat = H5Aread(id, atype, buf);
	if(stat < 0) goto error;

	str = string(buf);
	delete[] buf;

	H5Tclose(atype);
	H5Sclose(aspace);
	H5Aclose(id);

	if(ok!=NULL) *ok = true;
	return str;

	error:
	if(atype > 0) H5Tclose(atype);
	if(aspace > 0) H5Sclose(aspace);
	if(id > 0) H5Aclose(id);

	if (buf != NULL) delete[] buf;
	if(ok!=NULL) *ok = false;
	return "";
}


double* HDF5AttributeManager::readDoubleArray(std::string name, size_t *len, bool *ok) {
	bool temp_ok;
	if(ok == NULL) ok = &temp_ok;

	herr_t ret;
	hid_t attr = H5Aopen(this->parent->_id, name.c_str(), H5P_DEFAULT);
	if(attr < 0) {
		*ok = false;
		return NULL;
	}


	// Read space requirements
	hid_t aspace;
	ssize_t npoints;
	aspace = H5Aget_space(attr);
	if(aspace < 0) {
		H5Aclose(attr);
		*ok = false;
		return NULL;
	}
	npoints = (ssize_t)H5Sget_simple_extent_npoints(aspace);
	if(npoints < 0) {
		H5Sclose(aspace);
		H5Aclose(attr);
		*ok = false;
		return NULL;
	}

	// Get data points
	double* result = new double[npoints];
	ret = H5Aread(attr, H5T_NATIVE_DOUBLE, result );

	H5Sclose(aspace);
	H5Aclose(attr);

	if(ret == 0) {
		*ok = true;
		*len = npoints;
		return result;
	} else {
		delete[] result;
		*ok = false;
		return NULL;
	}
}


}

