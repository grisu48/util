/* =============================================================================
 *
 * Title:       Easy access to HDF5 files
 *                At this stage of the development the library supports only
 *                read access to HDF5
 * Author:      Felix Niederwanger
 * License:     MIT (http://opensource.org/licenses/MIT)
 * Description: Library, header file
 * 				This is part of the FlexLib2 Library found on GitHub
 * 				https://github.com/grisu48/FlexLib2
 * 				Modified for the needs of PICARD
 * =============================================================================
 */


// Standalone HDF5
#define _FLEXLIB_HDF5_STANDALONE 1


#ifndef _FLEXLIB_HDF5FILE_H
#define _FLEXLIB_HDF5FILE_H


// Include or exclude numeric matrix
#if _FLEXLIB_HDF5_STANDALONE != 1
#ifndef _FLEXLIB_HDF5_INCLUDE_NUMERIC_MATRIX_
#define _FLEXLIB_HDF5_INCLUDE_NUMERIC_MATRIX_ 1
#endif
#endif



#include <string>
#include <cstdio>
#include <vector>
#include <exception>
#include <map>
#include <valarray>

#include <hdf5.h>
#include "numeric.hpp"



namespace hdf5 {

class HDF5File;
class HDF5Object;
class HDF5Exception;
class HDF5Group;
class HDF5Dataset;
class HDF5Attribute;
class HDF5AttributeManager;



/** General HDF5 exception  */
#if _FLEXLIB_HDF5_STANDALONE != 1
class HDF5Exception : public Exception {
#else
class HDF5Exception : public std::exception {
#endif
private:
    std::string _what;

public:
	/** Create empty HDF5 exception */
    HDF5Exception();
    /** Create hdf5 exception with error message */
    HDF5Exception(std::string what) { this->_what = what; }
    /** Create hdf5 exception with error message */
    HDF5Exception(const char* what) throw () { this->_what = std::string(what); }
    virtual ~HDF5Exception() throw () {}

    static void clearErrorStack();
    static void printStack(FILE* stream = ::stderr);

#if _FLEXLIB_HDF5_STANDALONE == 1
    virtual const char* what() const throw () {
    	return this->_what.c_str();
    }
#endif
};



/** Access to a HDF5 file */
class HDF5File
{
private:
    /** Opened objects */
    std::vector<HDF5Object*> _objects;
    /** Filename */
    std::string _filename;

    /** H5 file identifier */
    hid_t fid;

    /** Main group */
    HDF5Group *_rootGroup;

    /** Initializes this object */
    void init(const char* filename, bool readOnly = false);

protected:
	/** Remove object from object stack */
    void removeObject(HDF5Object *obj);
    /** Remove object from object stack */
    void removeObject(HDF5Object &obj) { this->removeObject(&obj); }
    /** Add object to object stack */
    void addObject(HDF5Object *obj);
public:
	/** Open HDF5 file
	  * @throws HDF5Exception Thrown if an error occurs while opening the file
	*/
    HDF5File(std::string filename, bool readOnly = false);
	/** Open HDF5 file
	  * @throws HDF5Exception Thrown if an error occurs while opening the file
	*/
    HDF5File(const char* filename, bool readOnly = false);
	/** Clone HDF5 file instance
	  * @throws HDF5Exception Thrown if an error occurs while opening the file
	*/
    HDF5File(HDF5File &file);
    virtual ~HDF5File();

	/** Close file. This is automaticall called when the instance is deleted */
    void close(void);
    /** @return only the name of the HDF5 file */
    std::string filename();
    /** @return the full pathname of the file */
    std::string pathname();

    /** Get group with the given name
     @throws HDF5Exception Thrown if an error occurs and if the dataset does not exists
    */
    HDF5Group* group(std::string name);
    /** Get root data group
     @throws HDF5Exception Thrown if an error occurs and if the dataset does not exists
    */
    HDF5Group* rootGroup();
    /** Get dataset with the given name
     @throws HDF5Exception Thrown if an error occurs and if the dataset does not exists
    */
    HDF5Dataset* dataset(std::string name);

    /**
     * Create a group with the given name
     * @throws HDF5Exception Thrown if an error occurs while creation
     * @returns Instance of the created group
     */
    HDF5Group* createGroup(std::string name);

    /**
     * Create new dataset at the given pathname
     * @param name Name or absolute path of the new dataset. If not an absolute path (begins with a '/'), a sub-dataset of root is created
     * @param nDims Number of dimensions (e.g. 3 for a 3D dataset)
     * @param dims Dimension array, must be of the size of nDims
     * @param flags additional creation flags for HDF5. Currently not yet used
     *
     * @throws HDF5Exception Thrown if an error occurs while create the dataset
     * @returns the opened, created dataset
     */
    HDF5Dataset* createDataset(std::string name, int nDims, size_t* dims, int flags = 0);

    friend class HDF5Object;
    friend class HDF5Group;
    friend class HDF5Dataset;
    friend class HDF5Attribute;
};


/**
 * Attribute manager
 */
class HDF5AttributeManager {
protected:
	/** Parent objects */
	HDF5Object *parent;

	/** Create new attribute manager instance */
	HDF5AttributeManager(HDF5Object*parent);
	HDF5AttributeManager();
	virtual ~HDF5AttributeManager();


public:

	/** Read attribute names */
	std::vector<std::string> names(void);

	/**
	 * Checks if the given attribute exists
	 * @param name Name of the attribute to check
	 * @return true if exists, false if not
	 */
	bool hasAttribute(const char* name);
	/**
	 * Checks if the given attribute exists
	 * @param name Name of the attribute to check
	 * @return true if exists, false if not
	 */
	bool hasAttribute(const std::string name);

	/**
	 * Fetch a new list with all attributes of this object. Each time
	 * @return List of all attributes within this object
	 */
	virtual std::vector<HDF5Attribute> attributes(void);

	/**
	 * Get the attribute identified by it's name. If no such attribute exists, NULL is returned
	 * @param name Attribute name
	 * @return new HDF5Attribute instance or NULL, if not such attribute exists
	 */
	HDF5Attribute attribute(std::string name);

	/**
	 * Get the attribute identified by it's name. If no such attribute exists, NULL is returned
	 * @param name Attribute name
	 * @return new HDF5Attribute instance or NULL, if not such attribute exists
	 */
	HDF5Attribute attribute(const char* name);

	/**
	 * Get the attribute identified by it's name. If no such attribute exists, NULL is returned
	 * @param name Attribute name
	 * @return new HDF5Attribute instance or NULL, if not such attribute exists
	 */
	HDF5Attribute operator[](std::string name);

	/**
	 * Get the attribute identified by it's name. If no such attribute exists, NULL is returned
	 * @param name Attribute name
	 * @return new HDF5Attribute instance or NULL, if not such attribute exists
	 */
	HDF5Attribute operator[](const char* name);

	/**
	 * Creates a new attribute with the given name and value
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 */
	virtual void create(const std::string name, const int value);

	/**
	 * Creates a new attribute with the given name and value
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 */
	virtual void create(const std::string name, const long value);

	/**
	 * Creates a new attribute with the given name and value
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 */
	virtual void create(const std::string name, const float value);

	/**
	 * Creates a new attribute with the given name and value
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 */
	virtual void create(const std::string name, const double value);

	/**
	 * Creates a new attribute with the given name and value.
	 * The length of the string is determined via strlen
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 */
	virtual void create(const std::string name, const char* value);

	/**
	 * Creates a new attribute with the given name and value
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 * @param len size of the char array to be written
	 */
	virtual void create(const std::string name, const char* value, const size_t len);

	/**
	 * Creates new attribute array with the given name an value
	 * @param name Name of the attribute to be creates
	 * @param array Array that should be written
	 * @param len size of the int array to be written
	 */
	virtual void createArray(const std::string name, const int* array, const size_t len);

	/**
	 * Creates new attribute array with the given name an value
	 * @param name Name of the attribute to be creates
	 * @param array Array that should be written
	 * @param len size of the int array to be written
	 */
	virtual void createArray(const std::string name, const double* array, const size_t len);

	/**
	 * Creates a new attribute with the given name and value
	 * @param name Name of the attribute to be creates
	 * @param value Value that should be written
	 */
	virtual void create(const std::string name, const std::string value);

	/**
	 * Read integer value
	 * @param name Name of the attribute to read
	 * @param ok Boolean indicating if the read process was successful. Ignored if NULL
	 * @return read int or -1, if unsuccessful
	 */
	virtual int readInt(const std::string name, bool *ok=NULL);

	/**
	 * Read long value
	 * @param name Name of the attribute to read
	 * @param ok Boolean indicating if the read process was successful. Ignored if NULL
	 * @return read int or -1, if unsuccessful
	 */
	virtual long readLong(const std::string name, bool *ok=NULL);
	/**
	 * Read float value
	 * @param name Name of the attribute to read
	 * @param ok Boolean indicating if the read process was successful. Ignored if NULL
	 * @return read int or -1, if unsuccessful
	 */
	virtual float readFloat(const std::string name, bool *ok=NULL);

	/**
	 * Read double value
	 * @param name Name of the attribute to read
	 * @param ok Boolean indicating if the read process was successful. Ignored if NULL
	 * @return read int or -1, if unsuccessful
	 */
	virtual double readDouble(const std::string name, bool *ok=NULL);

	/**
	 * Read string value
	 * @param name Name of the attribute to read
	 * @param ok Boolean indicating if the read process was successful. Ignored if NULL
	 * @return read int or -1, if unsuccessful
	 */
	virtual std::string readString(const std::string name, bool *ok=NULL);

	/**
	 * Reads a double array. Returns NULL on error
	 * @param name Name of the attribute to read
	 * @param len Pointer to the size_t where the length of the array will be stored
	 * @param ok Boolean indicating if the read process was successful. Ignored if NULL
	 * @return a newly created double array of the size len or NULL, if an error occurred
	 */
	virtual double* readDoubleArray(std::string name, size_t *len, bool *ok=NULL);

	friend class HDF5Object;
	friend class HDF5Dataset;
	friend class HDF5Group;
	friend class HDF5Attribute;
};




/** Virtual class representing an object within a HDF5 file */
class HDF5Object {
protected:
    /** Object identifier*/
    hid_t _id;
    /** Parent file */
    HDF5File *_file;
    /** Internal pathname */
    std::string _pathname;
    /** File identifier */
    hid_t fid(void) { return _file->fid; }

    HDF5Object();
    HDF5Object(HDF5File *file);

    /** Type of this object
     * See the TYPE_ identifies in the public section of this class
     * */
    int _type;

    /**
     * Deletes the link to the given object
     * @param name Name of the object whoms link should be removed
     */
    void linkDelete(const char* name);

public:
	/** Type ID for a group */
    static const int TYPE_GROUP = 0x1;
    /** Type ID for a dataset */
    static const int TYPE_DATASET = 0x2;
    /** Type ID for aan attribute */
    static const int TYPE_ATTRIBUTE = 0x4;
    /** Type ID for getting all datatypes */
    static const int TYPE_ALL = 0xFFFF;


    virtual ~HDF5Object(void);

	/** Close the given object. */
    virtual void close(void) {};
    /** @return true if the object is closed */
    virtual bool isClosed(void);
    /** @return true if the object is not closed */
    virtual bool isOpened(void);

	/** @return the names of all subitems */
    virtual std::vector<std::string> getItemNames(void);
    /** @return the names of all subitems of a given type */
    virtual std::vector<std::string> getItemNames(const int type);

	/** @return name of all sub-groups
	 * @throws HDF5Exception Thrown if an error occurs while opening the attributes
	 * */
    virtual std::vector<std::string> getSubGroups(void);
    /** @return name of all sub-datasets
     * @throws HDF5Exception Thrown if an error occurs while opening the attributes
     * */
    virtual std::vector<std::string> getSubDatasets(void);

	/** Opens a given groupname
	 * @param pathname name or absolute pathname of the group to open
	 * @return HDF5Group instance of the matching group
	 * @throws HDF5Exception Thrown if an error occurs while opening the group
	 * */
    virtual HDF5Group* openGroup(std::string pathname);
	/** Opens a given dataset
	 * @param pathname name or absolute pathname of the dataset to open
	 * @return HDF5Group instance of the matching group
	 * @throws HDF5Exception Thrown if an error occurs while opening the dataset
	 * */
    virtual HDF5Dataset* openDataset(std::string pathname);


    /** Type ID of this object */
    int type(void);

    /** Get the internal pathname of the object */
    std::string pathname(void);
    /** Get the pathname of the group with an ending / at the end*/
    std::string groupPathname(void);

	/** @return true if this object is a group */
    bool isGroup(void);
    /** @return true if this object is a dataset */
    bool isDataset(void);
    /** @return true if this object is an attribute */
    bool isAttribute(void);

    friend class HDF5Attribute;
	friend class HDF5AttributeManager;
};

/** HDF5 group */
class HDF5Group : public HDF5Object {
protected:
	/** Internal constructor to create a group from a HDF5 file */
    HDF5Group(HDF5File *file, std::string name);

	/** @return The relative path of the group */
    std::string relativePath(std::string name);

public:
    virtual ~HDF5Group() {
    	this->close();
    }
    /** Close HDF5 data group. This is implicitly called when the object is deleted */
    virtual void close(void);

    /** Attribute manager for the object */
    HDF5AttributeManager attrs;

    /** Name of the group */
    std::string name(void);

	/** Get subdataset with the given name
	 * @throws HDF5Exception Thrown if an error occurs or the dataset does not exists */
    HDF5Dataset* dataset(std::string name);
	/** Get sub-group with the given name
	 * @throws HDF5Exception Thrown if an error occurs or the dataset does not exists */
    HDF5Group* group(std::string name);

    /**
     * Create a group with the given name
     * @throws HDF5Exception Thrown if an error occurs while creation
     * @returns Instance of the created group
     */
    HDF5Group* createGroup(std::string name);

    /**
     * Create new dataset at the given pathname
     * @param name Name or absolute path of the new dataset. If not an absolute path (begins with a '/'), a sub-dataset of this group is created
     * @param nDims Number of dimensions (e.g. 3 for a 3D dataset)
     * @param dims Dimension array, must be of the size of nDims
     * @param flags additional creation flags for HDF5
     * @throws HDF5Exception Thrown if an error occurs while create the dataset
     * @returns the opened, created dataset
     */
    HDF5Dataset* createDataset(std::string name, int nDims, size_t* dims, int flags = 0);

    friend class HDF5File;
};

class HDF5Dataset : public HDF5Object {
protected:
	/** Datatype of the dataset */
    hid_t       d_datatype;
    /** Class type */
    H5T_class_t d_class;
    /** Ordering */
    H5T_order_t d_order;
    /** Total size of the dataset */
    size_t      d_size;

	/** Rank (i.e. number of dimensions) */
    int         d_rank;
    /** Dimension size array */
    hsize_t    *d_dims;

	/** Internal constructor for creating a new dataset */
    HDF5Dataset(HDF5File *file, std::string pathname);

public:
    virtual ~HDF5Dataset();
    /** Close the dataset. This is implicitly called when the instance is deleted */
    virtual void close(void);
    /** Name of the dataset */
    std::string name(void);

    /** Attribute manager for the object */
    HDF5AttributeManager attrs;

	/** @return Size of the storage */
    long getStorageSize(void);

    /** The size in bytes of the datatype */
    size_t typeSize(void);
    /** Number of dimensions */
    size_t dims(void);
    /** Dimension size in given dimension */
    size_t dims(int dim);
    /** Total cell size */
    size_t cells(void);

    /** Total size of the whole dataset */
    size_t size(void);

    /** True if integer type dataset (INT, LONG, ... ) */
    bool isInteger(void);
    /** True if floating point type dataset (FLOAT, DOUBLE, ...) */
    bool isFloat(void);
    /** True if stored little endian */
    bool isLittleEndian(void);

    /** Read n double values from assumed 1d array */
    size_t read_1d(double* buf, const size_t n);

    /** Reads n-dimensional double values into the buffer.
    @param buf Destination buffer as 1d array
    @param n Number of dimensions to be read
    @param dims Dimension array, determining the number of cells in each dimension
    */
    size_t read(double *buf, const size_t n, const size_t* dims);

    /**
     * @brief read Reads a single datapoint out of the dataset
     * @param x X coordinate to be read
     * @param y Y coordinate to be read
     * @return Read double value
     */
    double read_2d(size_t x, size_t y);

    /**
     * @brief read Reads a single datapoint out of the dataset
     * @param x X coordinate to be read
     * @param y Y coordinate to be read
     * @return Read double value
     */
    double operator()(size_t x, size_t y);


    /** Read values into 1d double array
     * @param array pointer to the array, that should be created. The array will be created via the new keyword
     * @returns number of elements read from file
     */
    size_t read(double** array);

    /** Read values into 1d double array
     * @param array pointer to the array, that should be created. The array will be created via the new keyword
     * @returns number of elements read from file
     */
    size_t read(double*& array);

    /** Read datacube */
    numeric::Cube<double> readCube();
    /** Write datacube */
	void writeCube(const numeric::Cube<double> &cube);

	/**
	 * Write array
	 */
	void writeArray(std::valarray<double> &array);

	/**
	 * Writes the given double array of the size n to the dataset
	 * @param array to be written
	 * @param n Number of elements of the array
	 * @return number of elements written
	 */
	size_t write(double* array, size_t n);

    friend class HDF5File;
};

/** A HDF5 attribute */
class HDF5Attribute : public HDF5Object {
protected:
	/** The name of the attribute */
    std::string _name;
    /** The parent object of the attribute */
    HDF5Object* _parent;
    /** HDF5 attribute manager */
    HDF5AttributeManager* manager;

	/** Internal constructor for creating a new dataset */
    HDF5Attribute(const HDF5AttributeManager* manager, std::string name);

    /** (Re)opens the attribute.
     * If the attribute is already opened, this method as no effect
     */
    void open(void);

public:
    HDF5Attribute();
    HDF5Attribute(const HDF5Attribute&);
    virtual ~HDF5Attribute();

    /** Close attribute
     * @throws HDF5Exception Thrown if something happens while closing the attribute
     * */
    virtual void close(void);


	/** @return the name of the attribute */
    const std::string name(void);

    /**
     * Read the attribute as integer
     */
    int readInt(void);
    /**
     * Read the attribute as float
     */
    float readFloat(void);
    /**
     * Read the attribute as long
     */
    long readLong(void);
    /**
     * Read the attribute as double
     */
    double readDouble(void);




    friend class HDF5Object;
    friend class HDF5File;
    friend class HDF5AttributeManager;
};


}

#endif

