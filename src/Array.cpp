#include <iostream> // DEBUG
#include <stdexcept>
#include <cstring>
#include "Array.hpp"
#include "DebugHelper.hpp"

// #define COW_DISABLE_BOUNDS_CHECK

#define INDEX(i, j, k, m, n) (S[0] * i + S[1] * j + S[2] * k + S[3] * m + S[4] * n)
#define INDEX_ERROR(ii, nn) std::logic_error(#ii "=" + std::to_string (ii) + " not in bounds [0 " + std::to_string (nn) + ")")
#ifndef COW_DISABLE_BOUNDS_CHECK
#define BOUNDS_CHECK(i, j, k, m, n) do { \
if ( !(0 <= i && i < n1)) throw INDEX_ERROR(i, n1);\
if ( !(0 <= j && j < n2)) throw INDEX_ERROR(j, n2);\
if ( !(0 <= k && k < n3)) throw INDEX_ERROR(k, n3);\
if ( !(0 <= m && m < n4)) throw INDEX_ERROR(m, n4);\
if ( !(0 <= n && n < n5)) throw INDEX_ERROR(n, n5); } while (0)
#else
#define BOUNDS_CHECK(i, j, k, m, n) do { } while (0)
#endif

using namespace Cow;




// ============================================================================
std::ostream& operator<< (std::ostream &stream, const Cow::HeapAllocation &memory)
{
    stream.write (static_cast<const char*>(memory.begin()), memory.size());
    return stream;
}




// ============================================================================
HeapAllocation::HeapAllocation() : numberOfBytes (0)
{
    allocation = nullptr;
}

HeapAllocation::HeapAllocation (std::size_t numberOfBytes) : numberOfBytes (numberOfBytes)
{
    allocation = std::malloc (numberOfBytes);
}

HeapAllocation::HeapAllocation (const HeapAllocation& other) : numberOfBytes (other.numberOfBytes)
{
    allocation = std::malloc (other.numberOfBytes);
    std::memcpy (allocation, other.allocation, numberOfBytes);
}

HeapAllocation::HeapAllocation (std::string content) : numberOfBytes (content.size())
{
    allocation = std::malloc (numberOfBytes);
    std::memcpy (allocation, content.data(), numberOfBytes);
}

HeapAllocation::HeapAllocation (HeapAllocation&& other)
{
    allocation = other.allocation;
    numberOfBytes = other.numberOfBytes;
    other.allocation = nullptr;
    other.numberOfBytes = 0;
}

HeapAllocation::~HeapAllocation()
{
    std::free (allocation);
}

HeapAllocation& HeapAllocation::operator= (const HeapAllocation& other)
{
    if (&other != this)
    {
        numberOfBytes = other.numberOfBytes;
        allocation = std::realloc (allocation, numberOfBytes);
        std::memcpy (allocation, other.allocation, numberOfBytes);
    }
    return *this;
}

std::size_t HeapAllocation::size() const
{
    return numberOfBytes;
}

std::string HeapAllocation::toString() const
{
    auto message = std::string (numberOfBytes, 0);
    std::memcpy (&message[0], allocation, numberOfBytes);
    return message;
}

HeapAllocation HeapAllocation::swapBytes (std::size_t bytesPerEntry) const
{
    HeapAllocation M (numberOfBytes);
    const int numEntries = numberOfBytes / bytesPerEntry;

    for (int n = 0; n < numEntries; ++n)
    {
        const char* startSource = static_cast<const char*>(allocation) + n * bytesPerEntry;
        char* startTarget = static_cast<char*>(M.allocation) + n * bytesPerEntry;

        for (unsigned int b = 0; b < bytesPerEntry; ++b)
        {
            startTarget[bytesPerEntry - b - 1] = startSource[b];
        }
    }
    return M;
}




// ============================================================================
Range::Range (int lower, int upper, int stride) : lower (lower), upper (upper), stride (stride)
{

}

Range::Range (const char* colon) : lower (0), upper (0), stride (1)
{
    assert (std::strcmp (colon, ":") == 0);
}

bool Range::isRelative() const
{
    return upper <= 0 || lower < 0;
}

int Range::size () const
{
    assert (! isRelative());
    return (upper - lower) / stride;    
}

int Range::size (int absoluteSize) const
{
    return absolute (absoluteSize).size();
}

Range Range::absolute (int absoluteSize) const
{
    const int L = lower <  0 ? lower + absoluteSize : lower;
    const int U = upper <= 0 ? upper + absoluteSize : upper;
    return Range (L, U, stride);
}




// ============================================================================
Region Region::empty()
{
    Region R;

    for (int n = 0; n < 5; ++n)
    {
        R.lower[n] = 0;
        R.upper[n] = 0;
        R.stride[n] = 0;
    }
    return R;
}

Region Region::whole (Shape shape)
{
    return Region().absolute (shape);
}

Region::Region()
{
    for (int n = 0; n < 5; ++n)
    {
        lower[n] = 0;
        upper[n] = 0;
        stride[n] = 1;
    }
}

Region Region::withRange (int axis, int lower, int upper, int stride) const
{
    assert (axis < 5);
    auto R = *this;
    R.lower[axis] = lower;
    R.upper[axis] = upper;
    R.stride[axis] = stride;
    return R;
}

Region Region::withLower (int axis, int newLower) const
{
    assert (axis < 5);
    auto R = *this;
    R.lower[axis] = newLower;
    return R;
}

Region Region::withUpper (int axis, int newUpper) const
{
    assert (axis < 5);
    auto R = *this;
    R.upper[axis] = newUpper;
    return R;
}

Region Region::withStride (int axis, int newStride) const
{
    assert (axis < 5);
    auto R = *this;
    R.stride[axis] = newStride;
    return R;
}

bool Region::isRelative() const
{
    for (int n = 0; n < 5; ++n)
    {
        if (upper[n] <= 0) return true;
    }
    return false;
}

bool Region::isEmpty() const
{
    for (int n = 0; n < 5; ++n)
    {
        if (upper[n] != 0 || lower[n] != 0 || stride[n] != 0) return false;
    }
    return true;
}

bool Region::operator== (const Region& other) const
{
    return upper == other.upper && lower == other.lower && stride == other.stride;
}

Shape Region::shape() const
{
    return {{
        range (0).size(),
        range (1).size(),
        range (2).size(),
        range (3).size(),
        range (4).size() }};
}

int Region::size() const
{
    auto S = shape();
    return S[0] * S[1] * S[2] * S[3] * S[4];
}

std::vector<int> Region::getShapeVector() const
{
    return Array::vectorFromShape (shape());
}

Region Region::absolute (Shape shape) const
{
    Region R = *this;

    for (int n = 0; n < 5; ++n)
    {
        if (R.lower[n] <  0) R.lower[n] += shape[n];
        if (R.upper[n] <= 0) R.upper[n] += shape[n];
    }
    return R;
}

Region Region::absolute (std::vector<int> shapeVector) const
{
    Region R = *this;

    for (unsigned int n = 0; n < 5; ++n)
    {
        if (n < shapeVector.size())
        {
            if (R.lower[n] <  0) R.lower[n] += shapeVector[n];
            if (R.upper[n] <= 0) R.upper[n] += shapeVector[n];
        }
        else
        {
            R.lower[n] = 0;
            R.upper[n] = 1;
        }
    }
    return R;
}

void Region::ensureAbsolute (Shape shape)
{
    *this = absolute (shape);
}

Range Region::range (int axis) const
{
    return Range (lower[axis], upper[axis], stride[axis]);
}




// ============================================================================
Array::Array() : Array (0, 1, 1, 1, 1)
{

}

Array::Array (Shape shape) : Array (shape[0], shape[1], shape[2], shape[3], shape[4])
{

}

Array::Array (Reference reference)
{
    *this = reference.A.extract (reference.R);
}

Array::Array (int n1) : Array (n1, 1, 1, 1, 1)
{

}

Array::Array (int n1, int n2) : Array (n1, n2, 1, 1, 1)
{

}

Array::Array (int n1, int n2, int n3) : Array (n1, n2, n3, 1, 1)
{

}

Array::Array (int n1, int n2, int n3, int n4) : Array (n1, n2, n3, n4, 1)
{

}

Array::Array (int n1, int n2, int n3, int n4, int n5) :
n1 (n1),
n2 (n2),
n3 (n3),
n4 (n4),
n5 (n5),
memory (n1 * n2 * n3 * n4 * n5 * sizeof (double))
{
    for (int i = 0; i < size(); ++i)
    {
        memory.getElement<double> (i) = 0.0;
    }
    S[0] = n5 * n4 * n3 * n2;
    S[1] = n5 * n4 * n3;
    S[2] = n5 * n4;
    S[3] = n5;
    S[4] = 1;
}

Array::Array (const Array& other)
{
    memory = other.memory;
    S = other.S;
    n1 = other.n1;
    n2 = other.n2;
    n3 = other.n3;
    n4 = other.n4;
    n5 = other.n5;
}

Array::Array (Array&& other)
{
    memory = std::move (other.memory);
    S = other.S;
    n1 = other.n1;
    n2 = other.n2;
    n3 = other.n3;
    n4 = other.n4;
    n5 = other.n5;
    other = Array();
}

Array& Array::operator= (const Array& other)
{
    if (&other != this)
    {
        memory = other.memory;
        S = other.S;
        n1 = other.n1;
        n2 = other.n2;
        n3 = other.n3;
        n4 = other.n4;
        n5 = other.n5;
    }
    return *this;
}

int Array::size() const
{
    return n1 * n2 * n3 * n4 * n5;
}

int Array::size (int axis) const
{
    switch (axis)
    {
        case 0: return n1;
        case 1: return n2;
        case 2: return n3;
        case 3: return n4;
        case 4: return n5;
        default: assert (false); return 0;
    }
}

Shape Array::shape() const
{
    return {{n1, n2, n3, n4, n5}};
}

Shape Array::strides() const
{
    return S;
}

std::vector<int> Array::getShapeVector() const
{
    return vectorFromShape (shape());
}

Array Array::transpose() const
{
    auto A = Array (n5, n4, n3, n2, n1);

    for (int i = 0; i < n1; ++i)
    for (int j = 0; j < n2; ++j)
    for (int k = 0; k < n3; ++k)
    for (int m = 0; m < n4; ++m)
    for (int n = 0; n < n5; ++n)
    {
        A (n, m, k, j, i) = this->operator() (i, j, k, m, n);
    }
    return A;
}

Array Array::transpose (int axis1, int axis2) const
{
    auto sourceShape = shape();
    auto targetShape = shape();
    targetShape[axis1] = sourceShape[axis2];
    targetShape[axis2] = sourceShape[axis1];

    auto A = Array (targetShape);

    for (int i = 0; i < n1; ++i)
    for (int j = 0; j < n2; ++j)
    for (int k = 0; k < n3; ++k)
    for (int m = 0; m < n4; ++m)
    for (int n = 0; n < n5; ++n)
    {
        auto sourceIndex = Index {{i, j, k, m, n}};
        auto targetIndex = Index {{i, j, k, m, n}};
        targetIndex[axis1] = sourceIndex[axis2];
        targetIndex[axis2] = sourceIndex[axis1];

        const int it = targetIndex[0];
        const int jt = targetIndex[1];
        const int kt = targetIndex[2];
        const int mt = targetIndex[3];
        const int nt = targetIndex[4];

        A (it, jt, kt, mt, nt) = this->operator() (i, j, k, m, n);
    }
    return A;
}

double& Array::operator[] (int index)
{
    assert (0 <= index && index < size());
    return memory.getElement<double> (index);
}

const double& Array::operator[] (int index) const
{
    assert (0 <= index && index < size());
    return memory.getElement<double> (index);
}

Array::Reference Array::operator[] (Region R)
{
    return Reference (*this, R.absolute (shape()));
}

double& Array::operator() (int i)
{
    BOUNDS_CHECK(i, 0, 0, 0, 0);
    return memory.getElement<double> (INDEX(i, 0, 0, 0, 0));
}

double& Array::operator() (int i, int j)
{
    BOUNDS_CHECK(i, j, 0, 0, 0);
    return memory.getElement<double> (INDEX(i, j, 0, 0, 0));
}

double& Array::operator() (int i, int j, int k)
{
    BOUNDS_CHECK(i, j, k, 0, 0);
    return memory.getElement<double> (INDEX(i, j, k, 0, 0));
}

double& Array::operator() (int i, int j, int k, int m)
{
    BOUNDS_CHECK(i, j, k, m, 0);
    return memory.getElement<double> (INDEX(i, j, k, m, 0));
}

double& Array::operator() (int i, int j, int k, int m, int n)
{
    BOUNDS_CHECK(i, j, k, m, n);
    return memory.getElement<double> (INDEX(i, j, k, m, n));
}

const double& Array::operator() (int i) const
{
    BOUNDS_CHECK(i, 0, 0, 0, 0);
    return memory.getElement<double> (INDEX(i, 0, 0, 0, 0));
}

const double& Array::operator() (int i, int j) const
{
    BOUNDS_CHECK(i, j, 0, 0, 0);
    return memory.getElement<double> (INDEX(i, j, 0, 0, 0));
}

const double& Array::operator() (int i, int j, int k) const
{
    BOUNDS_CHECK(i, j, k, 0, 0);
    return memory.getElement<double> (INDEX(i, j, k, 0, 0));
}

const double& Array::operator() (int i, int j, int k, int m) const
{
    BOUNDS_CHECK(i, j, k, m, 0);
    return memory.getElement<double> (INDEX(i, j, k, m, 0));
}

const double& Array::operator() (int i, int j, int k, int m, int n) const
{
    BOUNDS_CHECK(i, j, k, m, n);
    return memory.getElement<double> (INDEX(i, j, k, m, n));
}

Array Array::extract (Region R) const
{
    R.ensureAbsolute (shape());
    auto A = Array (R.shape());
    copyRegion (A, *this, Region::whole (shape()), R);
    return A;
}

void Array::insert (const Array& source, Region R)
{
    R.ensureAbsolute (shape());

    if (source.shape() != R.shape())
    {
        throw std::logic_error ("source and target regions have different shapes");
    }
    copyRegion (*this, source, R, Region::whole (shape()));
}

void Array::copyFrom (const Array&A, Region source, Region target)
{
    source.ensureAbsolute (A.shape());
    target.ensureAbsolute (shape());

    if (source.shape() != target.shape())
    {
        throw std::logic_error ("source and target regions have different shapes");
    }
    copyRegion (*this, A, target, source);
}

void Array::reshape (int n1_, int n2_, int n3_, int n4_, int n5_)
{
    if (size() != n1_ * n2_ * n3_ * n4_ * n5_)
    {
        throw std::logic_error ("reshape operation would change array size");
    }
    n1 = n1_;
    n2 = n2_;
    n3 = n3_;
    n4 = n4_;
    n5 = n5_;
    S[0] = n5 * n4 * n3 * n2;
    S[1] = n5 * n4 * n3;
    S[2] = n5 * n4;
    S[3] = n5;
    S[4] = 1;
}

void Array::copyRegion (Array& dst, const Array& src, Region R1, Region R0)
{
    assert (! R0.isRelative());
    assert (! R1.isRelative());

    for (int i0 = R0.lower[0], i1 = R1.lower[0]; i0 < R0.upper[0] && i1 < R1.upper[0]; i0 += R0.stride[0], i1 += R1.stride[0])
    for (int j0 = R0.lower[1], j1 = R1.lower[1]; j0 < R0.upper[1] && j1 < R1.upper[1]; j0 += R0.stride[1], j1 += R1.stride[1])
    for (int k0 = R0.lower[2], k1 = R1.lower[2]; k0 < R0.upper[2] && k1 < R1.upper[2]; k0 += R0.stride[2], k1 += R1.stride[2])
    for (int m0 = R0.lower[3], m1 = R1.lower[3]; m0 < R0.upper[3] && m1 < R1.upper[3]; m0 += R0.stride[3], m1 += R1.stride[3])
    for (int n0 = R0.lower[4], n1 = R1.lower[4]; n0 < R0.upper[4] && n1 < R1.upper[4]; n0 += R0.stride[4], n1 += R1.stride[4])
    {
        dst (i1, j1, k1, m1, n1) = src (i0, j0, k0, m0, n0);
    }
}

Shape Array::shapeFromVector (std::vector<int> shapeVector)
{
    if (shapeVector.size() > 5)
    {
        throw std::logic_error ("shape vector must have size <= 5");
    }
    return {{
        shapeVector.size() > 0 ? shapeVector[0] : 1,
        shapeVector.size() > 1 ? shapeVector[1] : 1,
        shapeVector.size() > 2 ? shapeVector[2] : 1,
        shapeVector.size() > 3 ? shapeVector[3] : 1,
        shapeVector.size() > 4 ? shapeVector[4] : 1,
    }};
}

std::vector<int> Array::vectorFromShape (Shape shape)
{
    int lastNonEmptyAxis = 4;

    while (shape[lastNonEmptyAxis] == 1 && lastNonEmptyAxis >= 1)
    {
        --lastNonEmptyAxis;
    }
    return std::vector<int> (&shape[0], &shape[lastNonEmptyAxis] + 1);
}

void Array::deploy (Shape shape, std::function<void (int i, int j, int k)> function)
{
    for (int i = 0; i < shape[0]; ++i)
    for (int j = 0; j < shape[1]; ++j)
    for (int k = 0; k < shape[2]; ++k)
    {
        function (i, j, k);
    }
}




// ============================================================================
Array::Reference::Reference (Array& A, Region R) : A (A), R (R)
{
    assert (! R.isRelative());
}

const Array& Array::Reference::operator= (const Array& source)
{
    A.insert (source, R);
    return source;
}

const Array::Reference& Array::Reference::operator= (const Array::Reference& source)
{
    A.insert (Array (source), R);
    return source;
}

const Array& Array::Reference::getArray() const
{
    return A;
}

const Region& Array::Reference::getRegion() const
{
    return R;
}

Shape Array::Reference::shape() const
{
    return R.shape();
}

std::vector<int> Array::Reference::getShapeVector() const
{
    return getRegion().getShapeVector();
}

Array::Iterator Array::Reference::begin()
{
    return Iterator (A, R);
}

Array::Iterator Array::Reference::end()
{
    return Iterator (A, R, true);
}




// ============================================================================
Array::Iterator::Iterator (Array& A, Region R, bool isEnd) : A (A), R (R)
{
    assert (! R.isRelative());

    currentIndex = R.lower;
    currentAddress = isEnd ? A.end() : getAddress();
}

double* Array::Iterator::operator++ ()
{
    Index& I = currentIndex;

    for (int n = 4; n >= 0; --n)
    {
        I[n] += R.stride[n];

        if (I[n] < R.upper[n])
        {
            return currentAddress = getAddress();
        }
        I[n] = R.lower[n];
    }
    return currentAddress = A.end();
}

double& Array::Iterator::operator[] (int offset)
{
    return A[currentAddress - A.begin() + offset];
}

Array::Iterator::operator double*() const
{
    return currentAddress;
}

bool Array::Iterator::operator== (const Iterator& other) const
{
    return currentAddress == other.currentAddress;
}

void Array::Iterator::print (std::ostream& stream) const
{
    const Index& I = currentIndex;
    stream << I[0] << " " << I[1] << " " << I[2] << " " << I[3] << " " << I[4] << std::endl;
}

Index Array::Iterator::index() const
{
    return currentIndex;
}

Index Array::Iterator::relativeIndex() const
{
    Index I = currentIndex;
    for (int n = 0; n < 5; ++n)
    {
        I[n] -= R.lower[n];
    }
    return I;
}

double* Array::Iterator::getAddress() const
{
    // Computing the index directly may be 10-20% faster than calling
    // Array::operator().
    const Index& I = currentIndex;
    const Shape& S = A.S;
    return &A.memory.getElement<double> (INDEX(I[0], I[1], I[2], I[3], I[4]));
}
