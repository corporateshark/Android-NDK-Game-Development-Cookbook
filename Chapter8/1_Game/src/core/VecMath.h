#pragma once

#include <cmath>

#ifndef scriptexport
#define scriptexport(X)
#endif

#ifndef scriptmethod
#define scriptmethod
#endif

namespace Linderdaum
{
	/// Linderdaum Math Library
	namespace Math
	{
#undef INFINITY
		const float INFINITY     = 1e30f;
		const float EPSILON      = 1e-8f;
		const float PI           = 3.14159265358979323846f;
		const float PI2          = 2.0f * PI;
		const float TWOPI        = PI2;
		const float HALFPI       = PI / 2.0f;
		const float SQRT2        = 1.41421356f;
		const float DTOR         = PI / 180.0f;
		const float RTOD         = 180.0f / PI;
		const float LOGHALF      = -0.693147f; // log(0.5)
		const float LOGHALFI     = -1.442695f;  // Inverse of log(0.5)

		inline float LAbs( float A )
		{
			return ( A > 0.0f ) ? A : -A;
		}

		inline float LMin( float A, float B )
		{
			return ( A < B ) ? A : B;
		}

		inline float LMax( float A, float B )
		{
			return ( A > B ) ? A : B;
		}

		inline float    DegToRad( float F )
		{
			return F * DTOR;
		}

		inline float    RadToDeg( float F )
		{
			return F * RTOD;
		}

		template <typename T> void SwapValues( T& V1, T& V2 )
		{
			T Tmp = V1;
			V1 = V2;
			V2 = Tmp;
		}

		template <typename T> T Clamp( const T Value, const T Min, const T Max )
		{
			if ( Value > Max ) { return Max; }

			if ( Value < Min ) { return Min; }

			return Value;
		}

		template <typename T> T ClampPeriodic( const T Value, const T Min, const T Max )
		{
		}

		#pragma region Misc functions usefull in texture generation

		template <class T> T Step( T a, T x )          { return ( T )( x >= a ); }
		template <class T> T Boxstep( T a, T b, T x )     { return Clamp( ( x - a ) / ( b - a ), 0, 1 ); }
		template <class T> T Pulse( T a, T b, T x )    { return ( T )( ( x >= a ) - ( x >= b ) ); }
		template <class T> T Cubic( T a )              { return a * a * ( 3 - 2 * a ); }
		template <class T, class FactorT> T Lerp( T a, T b, FactorT x )     { return ( a + x * ( b - a ) ); }

		inline int Floor( float a )                 { return ( ( int )a - ( a < 0 && a != ( int )a ) ); }
		inline int Ceil( float a )                  { return ( ( int )a + ( a > 0 && a != ( int )a ) ); }

		inline float Gamma( float a, float g )         { return powf( a, 1 / g ); }

		inline float Bias( float a, float b )          { return powf( a, logf( b ) * LOGHALFI ); }

		inline float Exposure( float l, float k )        { return ( 1 - expf( -l * k ) ); }

		inline float Gain( float a, float b )
		{
			if ( a <= EPSILON ) { return 0; }

			if ( a >= 1.0f - EPSILON ) { return 1; }

			float p = ( logf( 1.0f - b ) * LOGHALFI );

			return ( ( a < 0.5 ) ? powf( 2.0f * a, p ) : 1.0f - powf( 2.0f * ( 1.0f - a ), p ) ) * 0.5f;
		}

		/// Cubically-interpolated "smooth" transition from a to b
		inline float SmoothStep( float a, float b, float x )
		{
			if ( x <= a ) { return 0; }

			if ( x >= b ) { return 1; }

			return Cubic( ( x - a ) / ( b - a ) );
		}

		#pragma endregion

		/// Floating-point division remainder
		inline float Mod( float a, float b )
		{
			a -= b * floor( a / b );

			if ( a < 0 ) { a += b; }

			return a;
		}

		/// Division remainder (wrap around for negative values)
		inline int ModInt( int a, int b )
		{
			int r = a % b;
			return ( r < 0 ) ? r + b : r;
		}

		inline bool    IsPowerOf2( const int Num )
		{
			return ( Num & ( Num - 1 ) ) == 0;
		}

		inline unsigned int    GetNextPowerOf2( unsigned int Num )
		{
			Num |= ( Num >> 1 );
			Num |= ( Num >> 2 );
			Num |= ( Num >> 4 );
			Num |= ( Num >> 8 );
			Num |= ( Num >> 16 );

			return Num + 1;
		}

		inline bool IsMaskSet( int Value, int Mask )
		{
			return ( Value & Mask ) == Mask;
		}

		inline bool IsBitSet( int Value, int BitNumber )
		{
			int Mask = 1 << BitNumber;

			return ( Value & Mask ) == Mask;
		}

		inline int SetBit( int Value, int BitNumber )
		{
			int Mask = 1 << BitNumber;

			return Value | Mask;
		}

		inline int    IntPow( int A, int B )
		{
			return static_cast<int>( pow( static_cast<float>( A ), B ) );
		}

		template<class T>
		inline T Sign( T Val )
		{
			return ( Val > 0 ) ? Val : -Val;
		}

		inline float fsign( float Val )
		{
			return ( Val > 0.0f ) ? 1.0f : -1.0f;
		}

		inline float ClipAngleTo0_360( float Angle )
		{
			/*         double IntPart = 0;
			         double ClampedPart = modf( static_cast<double>(Angle), &IntPart );

			         return static_cast<float>( ClampedPart * 360.0 );
			*/
			return std::fmod( Angle, 360.0f );
		}

		inline float ClipAngleTo180( float angle )
		{
			return ClipAngleTo0_360( angle ) - 180.0f;
		}

		inline double Round( double x, int Digits )
		{
			double Power = pow( 10.0, Digits );

			return static_cast<double>( static_cast<int>( x * Power ) ) / Power;
		}

		// find angle1-angle2 clipping it to [0..360]
		inline float FindAngleDelta( float angle1, float angle2 )
		{
			/*
			         float delta = angle1 - angle2;

			         delta = ClipAngleTo0_360( delta );

			         if ( delta > 180.0 )
			         {
			            delta = delta - 360.0f;   // invert delta
			         }

			         return delta;
			*/

			float From = ClipAngleTo0_360( angle2 );
			float To   = ClipAngleTo0_360( angle1 );

			float Dist  = To - From;

			if ( Dist < -180.0f )
			{
				Dist += 360.0f;
			}
			else if ( Dist > 180.0f )
			{
				Dist -= 360.0f;
			}

			return Dist;
		}
	}
}

class LVector2;
class LVector3;
class LVector4;
class LVector2i;

inline LVector2  operator*( const float A, const LVector2&  B );
inline LVector2i operator*( const int A,   const LVector2i& B );

inline LVector3 operator*( const float A, const LVector3& B );
inline LVector3 operator/( const float A, const LVector3& B );
inline LVector4 operator*( const float A, const LVector4& B );

/// 2D vector
class LVector2
{
public:
	float x;
	float y;
public:
	LVector2(): x( 0 ), y( 0 ) {};
	LVector2( float lx, float ly ): x( lx ), y( ly ) {};
	LVector2( int lx, int ly ): x( static_cast<float>( lx ) ),
		y( static_cast<float>( ly ) ) {};
	explicit LVector2( const float lx ): x( lx ), y( lx ) {};
	explicit LVector2( const LVector2i& Vec );
	//
	// LVector2
	//
	inline float        operator[]( const int Index ) const { return ( &x )[Index]; };
	inline float&       operator[]( const int Index ) { return ( &x )[Index]; };
	inline LVector2     operator-() const { return LVector2( -x, -y ); }

	inline LVector2     operator-( const LVector2& Vec ) const { return LVector2( x - Vec.x, y - Vec.y ); }
	inline LVector2     operator+( const LVector2& Vec ) const { return LVector2( x + Vec.x, y + Vec.y ); }

	inline LVector2     operator*( const float A ) const { return LVector2( x * A, y * A ); }
	inline LVector2     operator/( const float A ) const { return LVector2( x / A, y / A ); }

	inline LVector2&    operator+=( const LVector2& Vec )
	{
		x += Vec.x;
		y += Vec.y;

		return *this;
	}

	inline LVector2&    operator-=( const LVector2& Vec )
	{
		x -= Vec.x;
		y -= Vec.y;

		return *this;
	}

	inline LVector2&    operator*=( const float A )
	{
		x *= A;
		y *= A;

		return *this;
	}

	inline LVector2&    operator*=( const LVector2& Vec )
	{
		x *= Vec.x;
		y *= Vec.y;

		return *this;
	}

	inline bool  operator == ( const LVector2& Vec ) const { return ( Vec.x == x ) && ( Vec.y == y ); }
	inline bool  operator != ( const LVector2& Vec ) const { return ( Vec.x != x ) || ( Vec.y != y ); }

	inline const float* ToFloatPtr() const { return &x; };
	inline float*       ToFloatPtr() { return &x; };

	/// Dot product
	inline float        Dot( const LVector2& Vec ) const { return ( x * Vec.x + y * Vec.y ); }

	/// Interpolate vetween two vectors
	void         Lerp( const LVector2& Vec1, const LVector2& Vec2, float Factor )
	{
		if ( Factor <= 0.0f )
		{
			( *this ) = Vec1;
		}
		else if ( Factor >= 1.0f )
		{
			( *this ) = Vec2;
		}
		else
		{
			( *this ) = Vec1 + Factor * ( Vec2 - Vec1 );
		}
	}

	/// Safe 2d vector normalization
	void         Normalize();

	inline LVector2 GetNormalized() const
	{
		LVector2 Vec( *this );
		Vec.Normalize();
		return Vec;
	};

	/// Calculate reflection vector (valid only for unit vectors)
	void Reflect( const LVector2& Normal )
	{
		( *this ) -=  ( 2.0f * Dot( Normal ) ) * Normal;
	}

	/// Get reflection vector (valid only for unit vectors)
	inline LVector2 GetReflected( const LVector2& Normal ) const
	{
		LVector2 Vec( *this );
		Vec.Reflect( Normal );
		return Vec;
	}

	/// Get orthogonal vector
	inline LVector2 GetOrthogonal() const { return LVector2( -y, x ); }

	/// Euclidean length
	inline float        Length() const { return sqrt( x * x + y * y ); };

	/// Squared length (for faster distance comparison)
	inline float        SqrLength() const { return x * x + y * y; };
};

/// 2D integer vector
class LVector2i
{
public:
	int x;
	int y;
public:
	LVector2i(): x( 0 ), y( 0 ) {};
	LVector2i( int lx, int ly ): x( lx ), y( ly ) {};
	explicit LVector2i( const int lx ): x( lx ), y( lx ) {};
	//
	// LVector2i
	//
	inline int  operator[]( const int Index ) const { return ( &x )[Index]; };
	inline int& operator[]( const int Index ) { return ( &x )[Index]; };

	inline LVector2i operator-( const LVector2i& Vec ) const { return LVector2i( x - Vec.x, y - Vec.y ); }
	inline LVector2i operator+( const LVector2i& Vec ) const { return LVector2i( x + Vec.x, y + Vec.y ); }

	inline bool operator==( const LVector2i& Vec ) const { return ( Vec.x == x ) && ( Vec.y == y ); }
	inline bool operator!=( const LVector2i& Vec ) const { return ( Vec.x != x ) || ( Vec.y != y ); }

	inline LVector2i operator*( const int A ) const { return LVector2i( x * A, y * A ); }
	inline LVector2i operator/( const int A ) const { return LVector2i( x / A, y / A ); }

	inline const int*    ToIntPtr() const { return &x; };
	inline int*          ToIntPtr() { return &x; };
	inline float         Length() const { return sqrt( static_cast<float>( x * x + y * y ) ); };
	inline int           SqrLength() const { return x * x + y * y; };
};

/// 3D vector
class LVector3
{
public:
	float x;
	float y;
	float z;
public:
	LVector3(): x( 0 ), y( 0 ), z( 0 ) {};
	LVector3( float lx, float ly, float lz ): x( lx ), y( ly ), z( lz ) {};
	LVector3( int lx, int ly, int lz ): x( static_cast<float>( lx ) ),
		y( static_cast<float>( ly ) ),
		z( static_cast<float>( lz ) ) {};
	LVector3( const LVector2& Vec, const float lz ): x( Vec.x ), y( Vec.y ), z( lz ) {};
	explicit LVector3( const LVector2& Vec ): x( Vec.x ), y( Vec.y ), z( 0 ) {};
	explicit LVector3( const float lx ): x( lx ), y( lx ), z( lx ) {};
	//
	// LVector3
	//
	inline float  operator[]( const int Index ) const { return ( &x )[Index]; }

	inline float& operator[]( const int Index ) { return ( &x )[Index]; }

	inline LVector3 operator-() const { return LVector3( -x, -y, -z ); }
	inline LVector3 operator+() const { return LVector3( +x, +y, +z ); }

	inline LVector3 operator*( const float A ) const { return LVector3( x * A, y * A, z * A ); }
	inline LVector3 operator/( const float A ) const { return LVector3( x / A, y / A, z / A ); }

	inline LVector3 operator/( const LVector3& Vec ) const { return LVector3( x / Vec.x, y / Vec.y, z / Vec.z ); }

	inline LVector3 operator-( const LVector3& Vec ) const { return LVector3( x - Vec.x, y - Vec.y, z - Vec.z ); }
	inline LVector3 operator+( const LVector3& Vec ) const { return LVector3( x + Vec.x, y + Vec.y, z + Vec.z ); }

	LVector3& operator*=( const float A )
	{
		x *= A;
		y *= A;
		z *= A;

		return *this;
	}

	LVector3& operator/=( const float A )
	{
		// NO CHECKS HERE: maximum speed.

		x /= A;
		y /= A;
		z /= A;

		return *this;
	}

	LVector3& operator+=( const LVector3& Vec )
	{
		x += Vec.x;
		y += Vec.y;
		z += Vec.z;

		return *this;
	}

	LVector3& operator-=( const LVector3& Vec )
	{
		x -= Vec.x;
		y -= Vec.y;
		z -= Vec.z;

		return *this;
	}

	inline bool operator==( const LVector3& Vec ) const { return ( Vec.x == x ) && ( Vec.y == y ) && ( Vec.z == z ); }
	inline bool operator!=( const LVector3& Vec ) const { return ( Vec.x != x ) || ( Vec.y != y ) || ( Vec.z != z ); }

	/// Per-component multiplication
	inline LVector3  operator*( const LVector3& Vec ) const { return LVector3( x * Vec.x, y * Vec.y, z * Vec.z ); }

	inline float Dot( const LVector3& Vec ) const { return ( x * Vec.x + y * Vec.y + z * Vec.z ); }
	inline LVector3 Cross( const LVector3& Vec ) const { return LVector3( y * Vec.z - z * Vec.y, z * Vec.x - x * Vec.z, x * Vec.y - y * Vec.x ); }

	LVector3      OrthogonalVector() const
	{
		LVector3 Result = *this;

		Result.Normalize();

		return Result.Cross( Result + LVector3( 1.0f, 2.0f, 3.0f ) );
	}

	inline const float* ToFloatPtr() const { return &x; }
	inline float*       ToFloatPtr()       { return &x; }

	inline LVector2     ToVector2() const  { return LVector2( x, y ); }

	void Lerp( const LVector3& Vec1, const LVector3& Vec2, float Factor )
	{
		if ( Factor <= 0.0f )
		{
			( *this ) = Vec1;
		}
		else if ( Factor >= 1.0f )
		{
			( *this ) = Vec2;
		}
		else
		{
			( *this ) = Vec1 + Factor * ( Vec2 - Vec1 );
		}
	}

	void     Normalize();

	inline   bool IsZeroVector( float Eps ) const { return ( fabs( x ) < Eps && fabs( y ) < Eps && fabs( z ) < Eps ); }

	inline LVector3 GetNormalized() const
	{
		LVector3 Vec( *this );
		Vec.Normalize();
		return Vec;
	};

	inline float Length() const { return sqrt( x * x + y * y + z * z ); }
	inline float SqrLength() const { return x * x + y * y + z * z; }

	/// Get the zero-based index of this vector's maximum component
	inline int GetMaximumComponentIndex() const { return ( x > y ) ? ( ( x > z ) ? 0 : 2 ) : ( ( y > z ) ? 1 : 2 ); }

	/// Get the zero-based index of this vector's minimum component
	inline int GetMinimumComponentIndex() const { return ( x < y ) ? ( ( x < z ) ? 0 : 2 ) : ( ( y < z ) ? 1 : 2 ); }

	/// Calculate reflection vector (valid only for unit vectors)
	void Reflect( const LVector3& Normal ) { ( *this ) -=  ( 2.0f * Dot( Normal ) ) * Normal; }

	/// Get reflection vector (valid only for unit vectors)
	inline LVector3 GetReflected( const LVector3& Normal ) const
	{
		LVector3 Vec( *this );
		Vec.Reflect( Normal );
		return Vec;
	}
};

/// 3D integer vector
class LVector3i
{
public:
	int x;
	int y;
	int z;
public:
	LVector3i(): x( 0 ), y( 0 ), z( 0 ) {};
	LVector3i( int lx, int ly, int lz ): x( lx ), y( ly ), z( lz ) {};
	explicit LVector3i( const int lx ): x( lx ), y( lx ), z( lx ) {};
	//
	// LVector3i
	//
	inline int  operator[]( const int Index ) const { return ( &x )[Index]; }
	inline int& operator[]( const int Index )       { return ( &x )[Index]; }

	inline LVector3i operator-( const LVector3i& Vec ) const { return LVector3i( x - Vec.x, y - Vec.y, z - Vec.z ); }
	inline LVector3i operator+( const LVector3i& Vec ) const { return LVector3i( x + Vec.x, y + Vec.y, z + Vec.z ); }

	inline bool operator==( const LVector3i& Vec ) const { return ( Vec.x == x ) && ( Vec.y == y ) && ( Vec.z == z ); }
	inline bool operator!=( const LVector3i& Vec ) const { return ( Vec.x != x ) || ( Vec.y != y ) || ( Vec.z != z ); }

	inline LVector3i operator*( const int A ) const { return LVector3i( x * A, y * A, z * A ); }
	inline LVector3i operator/( const int A ) const { return LVector3i( x / A, y / A, z / A ); }

	inline const int*    ToIntPtr() const { return &x; }
	inline int*          ToIntPtr()       { return &x; }

	inline float         Length() const { return sqrt( static_cast<float>( x * x + y * y + z * z ) ); }
	inline int           SqrLength() const { return x * x + y * y + z * z; }
};

/// 4D int vector
class LVector4i
{
public:
	int x;
	int y;
	int z;
	int w;
public:
	LVector4i(): x( 0 ), y( 0 ), z( 0 ), w( 0 ) {};
	LVector4i( int lx, int ly, int lz, int lw ): x( lx ), y( ly ), z( lz ), w( lw ) {};
	explicit LVector4i( const int lx ): x( lx ), y( lx ), z( lx ), w( lx ) {};
	//
	// LVector4i
	//
	inline int  operator[]( const int Index ) const { return ( &x )[Index]; }
	inline int& operator[]( const int Index )       { return ( &x )[Index]; }

	inline bool operator==( const LVector4i& Vec ) const { return ( Vec.x == x ) && ( Vec.y == y ) && ( Vec.z == z ) && ( Vec.w == w ); }
	inline bool operator!=( const LVector4i& Vec ) const { return ( Vec.x != x ) || ( Vec.y != y ) || ( Vec.z != z ) || ( Vec.w != w ); }

	inline LVector2i    XY() const { return LVector2i( x, y ); }
	inline LVector2i    YX() const { return LVector2i( y, x ); }
	inline LVector3i    XYZ() const { return LVector3i( x, y, z ); }
	inline LVector3i    ZYX() const { return LVector3i( z, y, x ); }
};

/// 4D vector
class LVector4
{
public:
	float x;
	float y;
	float z;
	float w;
public:
	LVector4(): x( 0 ), y( 0 ), z( 0 ), w( 0 ) {};
	LVector4( float lx, float ly ): x( lx ), y( ly ), z( 0 ), w( 0 ) {};
	LVector4( float lx, float ly, float lz, float lw ): x( lx ), y( ly ), z( lz ), w( lw ) {};
	LVector4( int lx, int ly, int lz, int lw ): x( static_cast<float>( lx ) ),
		y( static_cast<float>( ly ) ),
		z( static_cast<float>( lz ) ),
		w( static_cast<float>( lw ) ) {};
	LVector4( const LVector3& Vec, const float lw ): x( Vec.x ), y( Vec.y ), z( Vec.z ), w( lw ) {};
	explicit LVector4( const float lx ): x( lx ), y( lx ), z( lx ), w( lx ) {};
	explicit LVector4( const LVector2& Vec ): x( Vec.x ), y( Vec.y ), z( 0 ), w( 0 ) {};
	explicit LVector4( const LVector3& Vec ): x( Vec.x ), y( Vec.y ), z( Vec.z ), w( 0 ) {};
	//
	// LVector4
	//
	inline float     operator[]( const int Index ) const { return ( &x )[Index]; }
	inline float&    operator[]( const int Index )       { return ( &x )[Index]; }

	inline LVector4  operator-( const LVector4& Vec ) const { return LVector4( x - Vec.x, y - Vec.y, z - Vec.z, w - Vec.w ); }
	inline LVector4  operator+( const LVector4& Vec ) const { return LVector4( x + Vec.x, y + Vec.y, z + Vec.z, w + Vec.w ); }

	inline LVector4  operator*( const float A ) const { return LVector4( x * A, y * A, z * A, w * A ); }
	inline LVector4  operator/( const float A ) const { return LVector4( x / A, y / A, z / A, w / A ); }

	inline LVector4& operator*=( const float A )
	{
		x *= A;
		y *= A;
		z *= A;
		w *= A;

		return *this;
	}

	inline LVector4& operator/=( const float A )
	{
		// We DO check for zero explicitly. This operator is used only three times in the engine
		x /= A;
		y /= A;
		z /= A;
		w /= A;

		return *this;
	}

	inline LVector4& operator -= ( const LVector4& Vec )
	{
		x -= Vec.x;
		y -= Vec.y;
		z -= Vec.z;
		w -= Vec.w;

		return *this;
	}

	inline LVector4& operator += ( const LVector4& Vec )
	{
		x += Vec.x;
		y += Vec.y;
		z += Vec.z;
		w += Vec.w;

		return *this;
	}

	inline bool operator==( const LVector4& Vec ) const { return ( Vec.x == x ) && ( Vec.y == y ) && ( Vec.z == z ) && ( Vec.w == w ); }
	inline bool operator!=( const LVector4& Vec ) const { return ( Vec.x != x ) || ( Vec.y != y ) || ( Vec.z != z ) || ( Vec.w != w ); }

	/// Per-component multiplication
	inline LVector4 operator*( const LVector4& Vec ) const { return LVector4( x * Vec.x, y * Vec.y, z * Vec.z, w * Vec.w ); }

	/// Dot product
	inline float Dot( const LVector4& Vec ) const { return ( x * Vec.x + y * Vec.y + z * Vec.z + w * Vec.w ); }

	inline const  float* ToFloatPtr() const { return &x; }
	inline float*        ToFloatPtr()       { return &x; }

	inline LVector2      ToVector2() const  { return LVector2( x, y ); }
	inline LVector3      ToVector3() const  { return LVector3( x, y, z ); }

	void         Lerp( const LVector4& Vec1, const LVector4& Vec2, float Factor )
	{
		if ( Factor <= 0.0f )
		{
			( *this ) = Vec1;
		}
		else if ( Factor >= 1.0f )
		{
			( *this ) = Vec2;
		}
		else
		{
			( *this ) = Vec1 + Factor * ( Vec2 - Vec1 );
		}
	}

	void            Normalize();
	const LVector4& Saturate();

	inline float Length() const { return sqrt( x * x + y * y + z * z + w * w ); }
	inline float SqrLength() const { return x * x + y * y + z * z + w * w; }

	inline bool  IsZeroVector( float Eps ) const { return ( fabs( x ) < Eps && fabs( y ) < Eps && fabs( z ) < Eps && fabs( w ) < Eps ); }

	/// swizzlers
	inline LVector4    BGRA() const { return LVector4( z, y, x, w ); };

	inline LVector2    XY() const { return LVector2( x, y ); }
	inline LVector2    YX() const { return LVector2( y, x ); }
	inline LVector3    XYZ() const { return LVector3( x, y, z ); }
	inline LVector3    ZYX() const { return LVector3( z, y, x ); }
};

inline LVector2  operator*( const float A, const LVector2&  B ) { return LVector2 ( B.x * A, B.y * A ); }
inline LVector2i operator*( const int A,   const LVector2i& B ) { return LVector2i( B.x * A, B.y * A ); }

inline LVector3 operator*( const float A, const LVector3& B ) { return LVector3( B.x * A, B.y * A, B.z * A ); }
inline LVector3 operator/( const float A, const LVector3& B ) { return LVector3( B.x / A, B.y / A, B.z / A ); }
inline LVector4 operator*( const float A, const LVector4& B ) { return LVector4( B.x * A, B.y * A, B.z * A, B.w * A ); }

/// Utility methods

inline float LVector3_MixedProduct( const LVector3& A, const LVector3& B, const LVector3& C )
{
	return A.Dot( B.Cross( C ) );
}

inline bool LVector3_AreCollinear( const LVector3& A, const LVector3& B, const LVector3& C, float Eps )
{
	return ( B - A ).Cross( C - A ).SqrLength() < Eps /*::Linderdaum::Math::EPSILON*/;
}

inline bool LVector3_AreComplanar( const LVector3& A, const LVector3& B, const LVector3& C, const LVector3& D, float Eps )
{
	return fabs( LVector3_MixedProduct( B - A, C - A, D - A ) ) < Eps /*::Linderdaum::Math::EPSILON*/;
}

// shortcuts
typedef LVector2 vec2;
typedef LVector3 vec3;
typedef LVector4 vec4;



namespace Linderdaum
{
	/// Linderdaum Math Library
	namespace Math
	{

		inline LVector3 SphericalToOrtho( float Radius, float Phi, float Theta )
		{
			return LVector3( Radius * cosf( Phi   * DTOR ),
			                 Radius * sinf( Phi   * DTOR ),
			                 Radius * sinf( Theta * DTOR ) );
		}

		inline float Vec2Angle( float x, float y )
		{
			return 180.0f + RadToDeg ( atan2( y, x ) );
			/*
			         if ( x >= 0 )
			         {
			            if ( y >= 0 )
			            {
			               // x > 0, y > 0
			               // first Q
			               return RadToDeg( acos( x ) );
			            }
			            else
			            {
			               // x > 0, y < 0
			               // fourth Q
			               return 360.0f - RadToDeg( acos( x ) );
			            }
			         }
			         else
			         {
			            if ( y >= 0 )
			            {
			               // x < 0, y > 0
			               // second Q
			               return 180.0f - RadToDeg( asin( y ) );
			            }
			            else
			            {
			               // x < 0, y < 0
			               // third Q
			               return 180.0f + RadToDeg( asin( -y ) );
			            }
			         }
			*/
		}

		inline LVector2 ToPolar( float RR, float Angle ) { return RR * vec2( cosf( DegToRad( Angle ) ), sinf( DegToRad( Angle ) ) ); }

		/**
		   Spherical and Cartesian coordinates

		   r     = |(x,y,z)|
		   phi   = arccos(y)
		   theta = atan2(x,z)

		   x = r * cos(theta) * sin(phi)
		   y = r * cos(theta) * cos(phi)
		   z = r * sin(theta)
		*/

		/// Convert (x,y,z) to (r, phi, theta)
		inline LVector3 CartesianToSpherical( const LVector3& Pos )
		{
			LVector3 Result;
			LVector3 NPos = Pos.GetNormalized();

			Result.x = Pos.Length();
			Result.y = Math::Vec2Angle( NPos.x, NPos.y );
			Result.z = RadToDeg( acos( NPos.z ) );

			return Result;
		}

		/// Convert (r, phi, theta) to (x,y,z)
		inline LVector3 SphericalToCartesian( const LVector3& Sph )
		{
			float sinPhi   = sin( Sph.y );
			float cosPhi   = cos( Sph.y );
			float sinTheta = sin( Sph.z );
			float cosTheta = cos( Sph.z );

			return Sph.x * LVector3( cosTheta * sinPhi, cosTheta * cosPhi, sinTheta );
		}

		inline LVector3 Barycentric2D( float x, float y, float x1, float y1, float x2, float y2, float x3, float y3 )
		{
			float detT = ( y2 - y3 ) * ( x1 - x3 ) + ( x3 - x2 ) * ( y1 - y3 );

			float l1 = ( ( y2 - y3 ) * ( x - x3 ) + ( x3 - x2 ) * ( y - y3 ) ) / detT;
			float l2 = ( ( y3 - y1 ) * ( x - x3 ) + ( x1 - x3 ) * ( y - y3 ) ) / detT;

			return LVector3( l1, l2, 1.0f - l1 - l2 );
		}

		inline float FactorAdjust( float Color, float Factor, float IntensityMax, float Gamma )
		{
			return ( Color > 0.001f ) ? IntensityMax * pow( Color * Factor, Gamma ) : 0.0f;
		}

		/// http://miguelmoreno.net/sandbox/wavelengthtoRGB/
		/// and http://www.midnightkite.com/color.html
		/// Wavelength from 350 to 780
		inline LVector3 ColorFromWaveLength( float W )
		{
			float Gamma  = 1.0f;
			float Blue   = 0.0f;
			float Green  = 0.0f;
			float Red    = 0.0f;
			float Factor = 0.0f;

			if ( ( W >= 350.0f ) && ( W < 440.0f ) )
			{
				Red    = -( W - 440.0f ) / ( 440.0f - 350.0f );
				Blue   = 1.0f;
			}
			else if ( ( W >= 440.0f ) && ( W < 490.0f ) )
			{
				Green  = ( W - 440.0f ) / ( 490.0f - 440.0f );
				Blue   = 1.0f;
			}
			else if ( ( W >= 490.0f ) && ( W < 510.0f ) )
			{
				Green  = 1.0f;
				Blue   = -( W - 510.0f ) / ( 510.0f - 490.0f );
			}
			else if ( ( W >= 510.0f ) && ( W < 580.0f ) )
			{
				Red    = ( W - 510.0f ) / ( 580.0f - 510.0f );
				Green  = 1.0f;
			}
			else if ( ( W >= 580.0f ) && ( W < 645.0f ) )
			{
				Red    = 1.0f;
				Green  = -( W - 645.0f ) / ( 645.0f - 580.0f );
			}
			else if ( ( W >= 645.0f ) && ( W <= 780.0f ) )
			{
				Red    = 1.0f;
			}

			if ( ( W >= 350.0f ) && ( W < 420.0f ) )
			{
				Factor = 0.3f + 0.7f * ( W - 350.0f ) / ( 420.0f - 350.0f );
			}
			else if ( ( W >= 420.0f ) && ( W < 700.0f ) )
			{
				Factor = 1.0f;
			}
			else if ( ( W >= 700.0f ) && ( W <= 780.0f ) )
			{
				Factor = 0.3f + 0.7f * ( 780.0f - W ) / ( 780.0f - 700.0f );
			}

			Red   = FactorAdjust( Red,   Factor, 255.0f, Gamma );
			Green = FactorAdjust( Green, Factor, 255.0f, Gamma );
			Blue  = FactorAdjust( Blue,  Factor, 255.0f, Gamma );

			return vec3( Red, Green, Blue ) / 255.0f;
		}

		/// [0..360] angle to the red..blue "rainbow"
		inline LVector3 ColorFromAngle( float phi )
		{
			float startA = 0.0f;

			float dA = 10.0f;

			if ( phi < startA + dA || phi > 360.0f + startA - dA )
			{
				float t = 0.0f;

				if ( phi > startA + dA )
				{
					t = ( phi - startA + dA - 360.0f ) / ( 2.0f * dA );
				}
				else
				{
					t = ( phi - startA + dA ) / ( 2.0f * dA );
				}

				return t * ColorFromWaveLength( 350.0f ) + ( 1.0f - t ) * ColorFromWaveLength( 780.0f );
			}

			// map [startA + dA, 360 + startA - dA] to [0..360]

			float phiN = ( phi - dA - startA ) / ( 360.0f - 2 * dA );

			return ColorFromWaveLength( 780.0f + phiN * ( 350.0f - 780.0f ) );
		}

	}
}

//
// LMatrix3
//

/// 3x3 matrix
class LMatrix3
{
private:
	LVector3 FMatrix[3];
public:
	inline LMatrix3() {};
	inline explicit LMatrix3( const float A )
	{
		FMatrix[0] = LVector3( A );
		FMatrix[1] = LVector3( A );
		FMatrix[2] = LVector3( A );
	};
	inline explicit LMatrix3( const LVector3& X,
	                          const LVector3& Y,
	                          const LVector3& Z )
	{
		FMatrix[0] = X;
		FMatrix[1] = Y;
		FMatrix[2] = Z;
	};
	//
	// LMatrix3
	//
	inline LVector3&       operator[]( const int Index )
	{
		return FMatrix[Index];
	};
	inline const LVector3& operator[]( const int Index ) const
	{
		return FMatrix[Index];
	};
	inline LMatrix3        operator*( const LMatrix3& Matrix ) const;
	inline LMatrix3        operator+( const LMatrix3& Matrix ) const;
	inline LVector3        operator*( const LVector3& Vector ) const;
	inline void            ZeroMatrix()
	{
		for ( int i = 0; i <= 2; ++i )
		{
			FMatrix[i] = LVector3( 0 );
		}
	};
	inline void            IdentityMatrix()
	{
		ZeroMatrix();

		for ( int i = 0; i <= 2; ++i )
		{
			FMatrix[i][i] = 1.0f;
		}
	};
	inline void            RotateMatrixAxis( const float Angle, const LVector3& Axis );
	inline void            RotateMatrix( const LVector3& V1, const LVector3& V2 );
	void                   Inverse();
	LMatrix3               GetInversed() const;
	LMatrix3               GetTransposed() const;
	inline const float*    ToFloatPtr() const
	{
		return FMatrix[0].ToFloatPtr();
	};
	inline float*          ToFloatPtr()
	{
		return FMatrix[0].ToFloatPtr();
	};
	static const LMatrix3& Identity();

	/// Make an orthogonal matrix out of the given one
	void                   Orthonormalize();
	void                   CalculateEigenVectors( LVector3* V, float* D ) const;
};

LMatrix3 LMatrix3::operator+( const LMatrix3& Matrix ) const
{
	LMatrix3 Result;

	for ( int i = 0; i != 3; ++i )
	{
		for ( int j = 0; j != 3; ++j )
		{
			Result[i][j] = FMatrix[i][j] + Matrix[i][j];
		}
	}

	return Result;
}

LMatrix3 LMatrix3::operator*( const LMatrix3& Matrix ) const
{
	LMatrix3 Result;

	const float* M1Ptr = ToFloatPtr();
	const float* M2Ptr = Matrix.ToFloatPtr();
	float* RPtr        = Result.ToFloatPtr();

	for ( int i = 0; i != 3; ++i )
	{
		for ( int j = 0; j != 3; ++j )
		{
			*RPtr = M1Ptr[0] * M2Ptr[ 0 * 3 + j ] +
			        M1Ptr[1] * M2Ptr[ 1 * 3 + j ] +
			        M1Ptr[2] * M2Ptr[ 2 * 3 + j ];
			RPtr++;
		}

		M1Ptr += 3;
	}

	return Result;
}

LVector3 LMatrix3::operator*( const LVector3& Vector ) const
{
	return LVector3(
	          FMatrix[ 0 ].x * Vector.x + FMatrix[ 1 ].x * Vector.y + FMatrix[ 2 ].x * Vector.z,
	          FMatrix[ 0 ].y * Vector.x + FMatrix[ 1 ].y * Vector.y + FMatrix[ 2 ].y * Vector.z,
	          FMatrix[ 0 ].z * Vector.x + FMatrix[ 1 ].z * Vector.y + FMatrix[ 2 ].z * Vector.z
	       );
}

void LMatrix3::RotateMatrixAxis( const float Angle, const LVector3& Axis )
{
	float CosA = cos( Angle );
	float SinA = sin( Angle );

	LVector3 NAxis = Axis.GetNormalized();

	float Ax = NAxis.x;
	float Ay = NAxis.y;
	float Az = NAxis.z;

	float AxAx = Ax * Ax;
	float AxAy = Ax * Ay;
	float AxAz = Ax * Az;

	float AyAx = AxAy;
	float AyAy = Ay * Ay;
	float AyAz = Ay * Az;

	float AzAx = AxAz;
	float AzAy = AyAz;
	float AzAz = Az * Az;

	FMatrix[0][0] = AxAx + ( 1.0f - AxAx ) * CosA;
	FMatrix[0][1] = AxAy * ( 1.0f - CosA ) + Az * SinA;
	FMatrix[0][2] = AxAz * ( 1.0f - CosA ) - Ay * SinA;

	FMatrix[1][0] = AyAx * ( 1.0f - CosA ) - Az * SinA;
	FMatrix[1][1] = AyAy + ( 1.0f - AyAy ) * CosA;
	FMatrix[1][2] = AyAz * ( 1.0f - CosA ) + Ax * SinA;

	FMatrix[2][0] = AzAx * ( 1.0f - CosA ) + Ay * SinA;
	FMatrix[2][1] = AzAy * ( 1.0f - CosA ) - Ax * SinA;
	FMatrix[2][2] = AzAz + ( 1.0f - AzAz ) * CosA;
}

void LMatrix3::RotateMatrix( const LVector3& V1, const LVector3& V2 )
{
	LVector3 Vec1 = V1.GetNormalized();
	LVector3 Vec2 = V2.GetNormalized();

	LVector3 Axis = Vec1.Cross( Vec2 );

	float CosAngle, Angle;

	if ( Axis.Length() == 0 )
	{
		Axis = LVector3( 0.0f, 0.0f, 1.0f );
	}

	CosAngle = Vec1.Dot( Vec2 );

	Angle = acos( CosAngle );

	RotateMatrixAxis( Angle, Axis );
}

//
// LMatrix4
//

/// 4x4 matrix
class LMatrix4
{
private:
	LVector4 FMatrix[4];
public:
	inline LMatrix4() {};

	inline explicit LMatrix4( const float A )
	{
		FMatrix[0] = LVector4( A );
		FMatrix[1] = LVector4( A );
		FMatrix[2] = LVector4( A );
		FMatrix[3] = LVector4( A );
	};

	inline explicit LMatrix4( const LVector4& X,
	                          const LVector4& Y,
	                          const LVector4& Z,
	                          const LVector4& W )
	{
		FMatrix[0] = X;
		FMatrix[1] = Y;
		FMatrix[2] = Z;
		FMatrix[3] = W;
	};

	inline explicit LMatrix4( const float* Floats16 )
	{
		for ( int i = 0; i != 4; i++ ) for ( int j = 0; j != 4; j++ )
			{
				FMatrix[i][j] = Floats16[i * 4 + j];
			}
	}

	inline explicit LMatrix4( const LMatrix3& mtx3 )
	{
		for ( int i = 0 ; i < 3 ; i++ ) for ( int j = 0 ; j < 3 ; j++ )
			{
				FMatrix[i][j] = mtx3[i][j];
			}

		FMatrix[0][3] = FMatrix[1][3] = FMatrix[2][3] = 0;
		FMatrix[3][0] = FMatrix[3][1] = FMatrix[3][2] = 0;
		FMatrix[3][3] = 1;
	}
	//
	// LMatrix4
	//
	inline LVector4&       operator[]( const int Index )
	{
		return FMatrix[Index];
	};
	inline const LVector4& operator[]( const int Index ) const
	{
		return FMatrix[Index];
	};
	inline LMatrix4        operator*( const LMatrix4& Matrix ) const;
	inline LVector4        operator*( const LVector4& Vector ) const;
	inline LVector3        operator*( const LVector3& Vector ) const;
	inline void            ZeroMatrix()
	{
		for ( int i = 0; i <= 3; ++i )
		{
			FMatrix[i] = LVector4( 0 );
		}
	};
	inline void            IdentityMatrix()
	{
		ZeroMatrix();

		for ( int i = 0; i <= 3; ++i )
		{
			FMatrix[i][i] = 1.0f;
		}
	};
	inline void            TranslateMatrix( const LVector3& Vector );
	inline void            ScaleMatrix( const LVector3& Vector );
	inline void            RotateMatrixAxis( const float Angle, const LVector3& Axis );
	inline void            RotateMatrix( const LVector3& V1, const LVector3& V2 );
	bool                   IsIdentityMatrix() const;
	void                   Inverse();
	LMatrix4               GetInversed() const;
	inline void            Transpose();
	inline LMatrix4        GetTransposed() const;
	LMatrix3               ExtractMatrix3() const;
	void                   SetSubMatrix( const LMatrix3& Mtx );
	float                  Det() const;
	inline const float*    ToFloatPtr() const
	{
		return FMatrix[0].ToFloatPtr();
	};
	inline float*          ToFloatPtr()
	{
		return FMatrix[0].ToFloatPtr();
	};

	/// Extract 3x3 part
	LMatrix3               ToMatrix3() const;

	/// Obsolete. Use LTransform::euler_angles instead
	LMatrix4               FromPitchPanRoll( float Pitch, float Pan, float Roll );

	#pragma region Static constructors for typical matrices

	/// Identity matrix (don't use for global variables initialization)
	static const LMatrix4& Identity();

	/// Identity matrix
	static LMatrix4        IdentityStatic();

	/// Translation matrix
	static LMatrix4        GetTranslateMatrix( const LVector3& Vector );

	/// Scaling matrix
	static LMatrix4        GetScaleMatrix( const LVector3& Vector );

	/// Rotation matrix for (axis,angle) pair
	static LMatrix4        GetRotateMatrixAxis( const float Angle, const LVector3& Axis );

	/// Diag * Identity() matrix
	static LMatrix4        GetDiagonalMatrix( float Diag );

	/// Form diagonal matrix from vector components
	static LMatrix4        GetDiagonalMatrixV( const LVector4& Diag );

	static LMatrix4        GetFromPitchPanRoll( float Pitch, float Pan, float Roll );

	#pragma endregion
};

inline bool operator== ( const LMatrix4& M1, const LMatrix4& M2 )
{
	const float* M1Ptr = M1.ToFloatPtr();
	const float* M2Ptr = M2.ToFloatPtr();

	for ( int i = 0; i != 16; ++i )
	{
		if ( M1Ptr[i] != M2Ptr[i] )
		{
			return false;
		}
	}

	return true;
}

inline bool operator!= ( const LMatrix4& M1, const LMatrix4& M2 )
{
	const float* M1Ptr = M1.ToFloatPtr();
	const float* M2Ptr = M2.ToFloatPtr();

	for ( int i = 0; i != 16; ++i )
	{
		if ( M1Ptr[i] != M2Ptr[i] )
		{
			return true;
		}
	}

	return false;
}

LMatrix4 LMatrix4::operator*( const LMatrix4& Matrix ) const
{
	LMatrix4 Result;

	const float* M1Ptr = ToFloatPtr();
	const float* M2Ptr = Matrix.ToFloatPtr();
	float* RPtr        = Result.ToFloatPtr();

//#pragma omp parallel for shared(RPtr)
	for ( int i = 0; i < 4; ++i )
	{
		for ( int j = 0; j < 4; ++j )
		{
			*RPtr = M1Ptr[0] * M2Ptr[ 0 * 4 + j ] +
			        M1Ptr[1] * M2Ptr[ 1 * 4 + j ] +
			        M1Ptr[2] * M2Ptr[ 2 * 4 + j ] +
			        M1Ptr[3] * M2Ptr[ 3 * 4 + j ];
			RPtr++;
		}

		M1Ptr += 4;
	}

	return Result;
}

LVector4 LMatrix4::operator*( const LVector4& Vector ) const
{
	return LVector4(
	          FMatrix[ 0 ].x * Vector.x + FMatrix[ 1 ].x * Vector.y + FMatrix[ 2 ].x * Vector.z + FMatrix[ 3 ].x * Vector.w,
	          FMatrix[ 0 ].y * Vector.x + FMatrix[ 1 ].y * Vector.y + FMatrix[ 2 ].y * Vector.z + FMatrix[ 3 ].y * Vector.w,
	          FMatrix[ 0 ].z * Vector.x + FMatrix[ 1 ].z * Vector.y + FMatrix[ 2 ].z * Vector.z + FMatrix[ 3 ].z * Vector.w,
	          FMatrix[ 0 ].w * Vector.x + FMatrix[ 1 ].w * Vector.y + FMatrix[ 2 ].w * Vector.z + FMatrix[ 3 ].w * Vector.w
	       );
}

LVector3 LMatrix4::operator*( const LVector3& Vector ) const
{
	return LVector3(
	          FMatrix[ 0 ].x * Vector.x + FMatrix[ 1 ].x * Vector.y + FMatrix[ 2 ].x * Vector.z + FMatrix[ 3 ].x,
	          FMatrix[ 0 ].y * Vector.x + FMatrix[ 1 ].y * Vector.y + FMatrix[ 2 ].y * Vector.z + FMatrix[ 3 ].y,
	          FMatrix[ 0 ].z * Vector.x + FMatrix[ 1 ].z * Vector.y + FMatrix[ 2 ].z * Vector.z + FMatrix[ 3 ].z
	       );
}

void LMatrix4::TranslateMatrix( const LVector3& Vector )
{
	IdentityMatrix();

	FMatrix[3] = LVector4( Vector );
	FMatrix[3][3] = 1.0f;
}

void LMatrix4::ScaleMatrix( const LVector3& Vector )
{
	ZeroMatrix();

	FMatrix[0][0] = Vector.x;
	FMatrix[1][1] = Vector.y;
	FMatrix[2][2] = Vector.z;
	FMatrix[3][3] = 1.0f;
}

void LMatrix4::RotateMatrixAxis( const float Angle, const LVector3& Axis )
{
	float CosA = cos( Angle );
	float SinA = sin( Angle );

	LVector3 NAxis = Axis.GetNormalized();

	float Ax = NAxis.x;
	float Ay = NAxis.y;
	float Az = NAxis.z;

	float AxAx = Ax * Ax;
	float AxAy = Ax * Ay;
	float AxAz = Ax * Az;

	float AyAx = AxAy;
	float AyAy = Ay * Ay;
	float AyAz = Ay * Az;

	float AzAx = AxAz;
	float AzAy = AyAz;
	float AzAz = Az * Az;

	FMatrix[0][0] = AxAx + ( 1.0f - AxAx ) * CosA;
	FMatrix[0][1] = AxAy * ( 1.0f - CosA ) + Az * SinA;
	FMatrix[0][2] = AxAz * ( 1.0f - CosA ) - Ay * SinA;
	FMatrix[0][3] = 0;

	FMatrix[1][0] = AyAx * ( 1.0f - CosA ) - Az * SinA;
	FMatrix[1][1] = AyAy + ( 1.0f - AyAy ) * CosA;
	FMatrix[1][2] = AyAz * ( 1.0f - CosA ) + Ax * SinA;
	FMatrix[1][3] = 0;

	FMatrix[2][0] = AzAx * ( 1.0f - CosA ) + Ay * SinA;
	FMatrix[2][1] = AzAy * ( 1.0f - CosA ) - Ax * SinA;
	FMatrix[2][2] = AzAz + ( 1.0f - AzAz ) * CosA;
	FMatrix[2][3] = 0;

	FMatrix[3][0] = 0;
	FMatrix[3][1] = 0;
	FMatrix[3][2] = 0;
	FMatrix[3][3] = 1.0f;
}

void LMatrix4::RotateMatrix( const LVector3& V1, const LVector3& V2 )
{
	LVector3 Vec1 = V1.GetNormalized();
	LVector3 Vec2 = V2.GetNormalized();

	LVector3 Axis = Vec1.Cross( Vec2 );

	float CosAngle, Angle;

	if ( Axis.Length() == 0 )
	{
		Axis = LVector3( 0.0f, 0.0f, 1.0f );
	}

	CosAngle = Vec1.Dot( Vec2 );

	Angle = acos( CosAngle );

	RotateMatrixAxis( Angle, Axis );
}

/// Associate a skew-symmetric matrix to the vector V
inline LMatrix3 VectorStar( const LVector3& V )
{
	LMatrix3 M;

	M[0][0] = 0.0f;
	M[1][0] = V.x;
	M[2][0] =  V.z;
	M[0][1] = -V.x;
	M[1][1] = 0.0f;
	M[2][1] = -V.y;
	M[0][2] = -V.z;
	M[1][2] = V.y;
	M[2][2] = 0.0f;

	return M;
}

/// M[i][j] = V1[i] * V2[j]
inline LMatrix3 TensorProduct_VecByVec( const LVector3& V1, const LVector3& V2 )
{
	LMatrix3 M;

	for ( int i = 0 ; i < 3 ; i++ )
	{
		for ( int j = 0 ; j < 3 ; j++ )
		{
			M[i][j] = V1[i] * V2[j];
		}
	}

	return M;
}

inline void  LMatrix4::Transpose()
{
	float t;
	int i, j;

	for ( i = 0 ; i < 4 ; i++ )
		for ( j = 0 ; j < 4 ; j++ )
		{ t = FMatrix[i][j]; FMatrix[i][j] = FMatrix[j][i]; FMatrix[j][i] = t; }
}

inline LMatrix4 LMatrix4::GetTransposed() const
{
	LMatrix4 Res;
	int i, j;

	for ( i = 0 ; i < 4 ; i++ )
		for ( j = 0 ; j < 4 ; j++ )
		{
			Res[i][j] = FMatrix[j][i];
		}

	return Res;
}

typedef LMatrix4 mtx4;
typedef LMatrix3 mtx3;

namespace Linderdaum
{
	namespace Math
	{

		scriptexport( Math ) inline LMatrix4 IdentityMatrix4()
		{
			static LMatrix4 IdMatrix;

			IdMatrix.IdentityMatrix();

			return IdMatrix;
		}

	} // namespace Math
} // namespace Linderdaum

class LQuaternion;

inline LQuaternion operator*( const LQuaternion& Q1, const LQuaternion& Q2 );
inline LQuaternion operator+( const LQuaternion& Q1, const LQuaternion& Q2 );
inline LQuaternion operator*( const float A, const LQuaternion& B );

/// Quaternion
class LQuaternion
{
public:
	LVector3    FVec;
	float       FW;
public:
	LQuaternion(): FVec( 0.0f ),
		FW( 1.0f ) {};

	LQuaternion( const LQuaternion& q ): FVec( q.FVec.x, q.FVec.y, q.FVec.z ), FW( q.FW ) {};

	LQuaternion( float x, float y, float z, float w ): FVec( x, y, z ), FW( w ) {};

	LQuaternion( const LVector3& Vec, float w ): FVec( Vec ),
		FW( w ) {};
	explicit LQuaternion( const LVector4& Vec ): FVec( Vec.ToVector3() ), FW( Vec.w ) {};

	explicit LQuaternion( const LMatrix3& Mtx ) { FromMatrix3( Mtx ); }
	explicit LQuaternion( const LMatrix4& Mtx ) { FromMatrix3( Mtx.ExtractMatrix3() ); }

	//
	// LQuaternion
	//
	inline LQuaternion& Conjugate()
	{
		FVec = -FVec;

		return *this;
	}

	inline LQuaternion&    operator =( const LQuaternion& Q )
	{
		FVec = Q.FVec;
		FW   = Q.FW;

		return *this;
	}

	inline LQuaternion&    operator+=( const LQuaternion& Q )
	{
		FVec += Q.FVec;
		FW   += Q.FW;

		return *this;
	}
	inline LQuaternion&    operator-=( const LQuaternion& Q )
	{
		FVec -= Q.FVec;
		FW   -= Q.FW;

		return *this;
	}
	inline LQuaternion&    operator*=( const LQuaternion& Q )
	{
		LQuaternion Q1 = *this;
		LQuaternion Q2 = Q;

		*this = LQuaternion(
		           Q1.FW * Q2.FVec.x + Q1.FVec.x * Q2.FW     + Q1.FVec.y * Q2.FVec.z - Q1.FVec.z * Q2.FVec.y,
		           Q1.FW * Q2.FVec.y + Q1.FVec.y * Q2.FW     + Q1.FVec.z * Q2.FVec.x - Q1.FVec.x * Q2.FVec.z,
		           Q1.FW * Q2.FVec.z + Q1.FVec.z * Q2.FW     + Q1.FVec.x * Q2.FVec.y - Q1.FVec.y * Q2.FVec.x,
		           Q1.FW * Q2.FW     - Q1.FVec.x * Q2.FVec.x - Q1.FVec.y * Q2.FVec.y - Q1.FVec.z * Q2.FVec.z );

		return *this;
		/*
		     LVector3 Vec( Q.FVec * FW + Q.FW * FVec + Q.FVec.Cross( FVec ) );

		     FW   = Q.FW * FW - Q.FVec*FVec;
		     FVec = Vec;

		     return *this;
		*/
	}
	inline LQuaternion&    operator*=( const float F )
	{
		FVec *= F;
		FW   *= F;

		return *this;
	}
	inline LQuaternion&    operator/=( const float F )
	{
		const float InvF = 1.0f / F;

		FVec *= InvF;
		FW   *= InvF;

		return *this;
	}

	void        Normalize()
	{
		LVector4 Vec( FVec, FW );

		Vec.Normalize();

		FVec = Vec.ToVector3();
		FW   = Vec.w;
	}

	void        ReNormalizeW()
	{
		float Wr = 1.0f - ( FVec.x * FVec.x ) - ( FVec.y * FVec.y ) - ( FVec.z * FVec.z );

		FW = ( Wr < 0.0f ) ? 0.0f : -sqrt( Wr );
	}

	LVector3 RotateVector( const LVector3& Vector ) const
	{
		LQuaternion p     ( Vector, 0.0f );
		LQuaternion qConj ( -FVec, FW );

		p  = *this * p * qConj;

		return p.FVec;
	}

	void        IdentityQuaternion()
	{
		FVec = LVector3( 0.0f );
		FW = 1.0f;
	}

	void        FromMatrix3( const LMatrix3& mtx )
	{
		float s0, s1, s2;
		int k0, k1, k2, k3;

		float m00 = mtx[0][0];
		float m11 = mtx[1][1];
		float m22 = mtx[2][2];

		if ( m00 + m11 + m22 > 0.0f )
		{
			k0 = 3;
			k1 = 2;
			k2 = 1;
			k3 = 0;
			s0 = s1 = s2 = 1.0f;
		}
		else if ( m00 > m11 && m00 > m22 )
		{
			k0 = 0;
			k1 = 1;
			k2 = 2;
			k3 = 3;
			s0 = 1.0f;
			s1 = -1.0f;
			s2 = -1.0f;
		}
		else if ( m11 > m22 )
		{
			k0 = 1;
			k1 = 0;
			k2 = 3;
			k3 = 2;
			s0 = -1.0f;
			s1 = 1.0f;
			s2 = -1.0f;
		}
		else
		{
			k0 = 2;
			k1 = 3;
			k2 = 0;
			k3 = 1;
			s0 = -1.0f;
			s1 = -1.0f;
			s2 = 1.0f;
		}

		float t = s0 * m00 + s1 * m11 + s2 * m22 + 1.0f;

		float s = /*ReciprocalSqrt( t )*/ 0.5f / sqrt( t );

		LVector4 vv;
		vv[k0] = s * t;
		vv[k1] = ( mtx[0][1] - s2 * mtx[1][0] ) * s;
		vv[k2] = ( mtx[2][0] - s1 * mtx[0][2] ) * s;
		vv[k3] = ( mtx[1][2] - s0 * mtx[2][1] ) * s;

		FVec.x = vv[0];
		FVec.y = vv[1];
		FVec.z = vv[2];
		FW = -vv[3]; // seems to be erroneous...
	}

	LMatrix3    ToMatrix3() const
	{
		LMatrix3 M;

		float wx, wy, wz;
		float xx, yy, yz;
		float xy, xz, zz;
		float x2, y2, z2;

		x2 = FVec.x + FVec.x;
		y2 = FVec.y + FVec.y;
		z2 = FVec.z + FVec.z;

		xx = FVec.x * x2;
		xy = FVec.x * y2;
		xz = FVec.x * z2;

		yy = FVec.y * y2;
		yz = FVec.y * z2;
		zz = FVec.z * z2;

		wx = FW * x2;
		wy = FW * y2;
		wz = FW * z2;

		M[ 0 ][ 0 ] = 1.0f - ( yy + zz );
		M[ 0 ][ 1 ] = xy - wz;
		M[ 0 ][ 2 ] = xz + wy;

		M[ 1 ][ 0 ] = xy + wz;
		M[ 1 ][ 1 ] = 1.0f - ( xx + zz );
		M[ 1 ][ 2 ] = yz - wx;

		M[ 2 ][ 0 ] = xz - wy;
		M[ 2 ][ 1 ] = yz + wx;
		M[ 2 ][ 2 ] = 1.0f - ( xx + yy );

		return M;
	}

	LVector4    ToVector4() const { return LVector4( FVec, FW ); }

	void        FromAxisAngle( const LVector3& Axis, const float Angle )
	{
		const float HalfAngle = Angle / 2.0f;

		FVec = Axis * sinf( HalfAngle );
		FW = cosf( HalfAngle );
	}

	void        ToAxisAngle( LVector3& Axis, float& Angle ) const
	{
		Angle = 2.0f * acosf( FW );
		Axis = ( FVec.SqrLength() > ::Linderdaum::Math::EPSILON ) ? FVec.GetNormalized() :
		       LVector3( 1.0f, 0.0f, 0.0f );
	}

	/**
	   \brief Spherical linear interpolation

	   Code from http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	*/
	void SLERP( const LQuaternion& qa, const LQuaternion& q2, float t )
	{
		LQuaternion qb = q2;
		// quaternion to return
		LQuaternion qm;

		// Calculate angle between them.
		double cosHalfTheta = qa.FVec.Dot( qb.FVec ) + qa.FW * qb.FW;

		// if qa=qb or qa=-qb then theta = 0 and we can return qa
		if ( fabs( cosHalfTheta ) >= 1.0 )
		{
			*this = qa;
			return;
		}

		if ( cosHalfTheta < 0 )
		{
			qb.FVec = LVector3( -qb.FVec.x, -qb.FVec.y, -qb.FVec.z );
			qb.FW = -qb.FW;
			cosHalfTheta = -cosHalfTheta;
		}

		// Calculate temporary values.
		double halfTheta = acos( cosHalfTheta );
		double sinHalfTheta = sqrt( 1.0 - cosHalfTheta * cosHalfTheta );

		// if theta = 180 degrees then result is not fully defined
		// we could rotate around any axis normal to qa or qb
		if ( fabs( sinHalfTheta ) < 0.001 )
		{
			*this = 0.5f * ( qa + qb );

			return;
		}

		float ratioA = static_cast<float>( sin( ( 1 - t ) * halfTheta ) / sinHalfTheta );
		float ratioB = static_cast<float>( sin( t * halfTheta ) / sinHalfTheta );
		//calculate Quaternion.

		*this = ( ratioA * qa + ratioB * qb );
	}
};

inline LQuaternion operator+( const LQuaternion& Q1,
                              const LQuaternion& Q2 )
{
	return LQuaternion( Q1.FVec + Q2.FVec,
	                    Q1.FW + Q2.FW );
}

inline LQuaternion operator-( const LQuaternion& Q1,
                              const LQuaternion& Q2 )
{
	return LQuaternion( Q1.FVec - Q2.FVec,
	                    Q1.FW - Q2.FW );
}

inline LQuaternion operator-( const LQuaternion& Q )
{
	return LQuaternion( -Q.FVec, -Q.FW );
}

inline LQuaternion operator*( const float A, const LQuaternion& B )
{
	return LQuaternion( A * B.FVec, A * B.FW );
}

inline LQuaternion operator*( const LQuaternion& Q1,
                              const LQuaternion& Q2 )
{
	/*
	   return LQuaternion( Q1.FVec * Q2.FW + Q1.FW * Q2.FVec + Q1.FVec.Cross( Q2.FVec ),
	                       Q1.FW * Q2.FW - Q1.FVec*Q2.FVec );
	*/
	return LQuaternion(
	          Q1.FW * Q2.FVec.x + Q1.FVec.x * Q2.FW     + Q1.FVec.y * Q2.FVec.z - Q1.FVec.z * Q2.FVec.y,
	          Q1.FW * Q2.FVec.y + Q1.FVec.y * Q2.FW     + Q1.FVec.z * Q2.FVec.x - Q1.FVec.x * Q2.FVec.z,
	          Q1.FW * Q2.FVec.z + Q1.FVec.z * Q2.FW     + Q1.FVec.x * Q2.FVec.y - Q1.FVec.y * Q2.FVec.x,
	          Q1.FW * Q2.FW     - Q1.FVec.x * Q2.FVec.x - Q1.FVec.y * Q2.FVec.y - Q1.FVec.z * Q2.FVec.z );

}

typedef LQuaternion quat;

/**
 INAR-encoding for Euler angles system
 (index - negation - alternate - reversal)

 i n a r  Sys.  i n a r  Sys.  i n a r  Sys.  i n a r  Sys.
(1,0,0,0) xzxs (1,0,1,0) xzys (1,1,0,0) xyxs (1,1,1,0) xyzs
(2,0,0,0) yxys (2,0,1,0) yxzs (2,1,0,0) yzys (2,1,1,0) yzxs
(3,0,0,0) zyzs (3,0,1,0) zyxs (3,1,0,0) zxzs (3,1,1,0) zxys
(1,0,0,1) xzxr (1,0,1,1) yzxr (1,1,0,1) xyxr (1,1,1,1) zyxr
(2,0,0,1) yxyr (2,0,1,1) zxyr (2,1,0,1) yzyr (2,1,1,1) xzyr
(3,0,0,1) zyzr (3,0,1,1) xyzr (3,1,0,1) zxzr (3,1,1,1) yxzr
*/

enum LEulerAngleSystem
{
   Euler_xzxs = 0, Euler_xzys, Euler_xyxs, Euler_xyzs,
   Euler_yxys, Euler_yxzs, Euler_yzys, Euler_yzxs,
   Euler_zyzs, Euler_zyxs, Euler_zxzs, Euler_zxys,
   Euler_xzxr, Euler_yzxr, Euler_xyxr, Euler_zyxr,
   Euler_yxyr, Euler_zxyr, Euler_yzyr, Euler_xzyr,
   Euler_zyzr, Euler_xyzr, Euler_zxzr, Euler_yxzr
};

enum LProjectionType
{
   PROJECTION_ORTHOGRAPHIC = 0,
   PROJECTION_PERSPECTIVE  = 1,
   PROJECTION_ERROR = 2
};

namespace Linderdaum
{
	namespace Math
	{
		scriptexport( Math )  LMatrix4    Ortho2D( float L, float R, float B, float T );
		scriptexport( Math )  LMatrix4    Ortho( float L, float R, float B, float T, float N, float F );
		scriptexport( Math )  LMatrix4    Perspective( float FOV, float Aspect, float NearCP, float FarCP );
		/// true - the left eye, false - the right eye
		scriptexport( Math )  LMatrix4    PerspectiveStereo( float FOV, float Aspect, float NearCP, float FarCP, float IOD, float FocalLength, bool WhichEye );
		scriptexport( Math )  LMatrix4    Frustum( float L, float R, float B, float T, float N, float F );
		scriptexport( Math )  LMatrix4    LookAt( LVector3 Eye, LVector3 Center, LVector3 Up );
		/// true - the left eye, false - the right eye
		scriptexport( Math )  LMatrix4    LookAtStereo( LVector3 Eye, LVector3 Center, LVector3 Up, float IOD, bool WhichEye );
		scriptexport( Math )  LMatrix4    LookAtStereoMatrix( const LMatrix4& View, float IOD, bool WhichEye );
		scriptexport( Math )  LVector3    GetViewingDirection( const LMatrix4& View );
		/// from left to right
		scriptexport( Math )  LVector3    GetSideDirection( const LMatrix4& View );
		scriptexport( Math )  LMatrix4    ProjectReflectionTexture( const LMatrix4& Projection, const LMatrix4& ModelView );
		scriptexport( Math )  LVector3    ProjectPoint( LVector3 Point, const LMatrix4& Projection, const LMatrix4& ModelView );
		scriptexport( Math )  LVector3    ProjectPointNDC( const LVector3& Point, const LMatrix4& Projection, const LMatrix4& ModelView );
		scriptexport( Math )  LVector3    UnProjectPoint( LVector3 Point, const LMatrix4& Projection, const LMatrix4& ModelView );
		scriptexport( Math )  LVector3    UnProjectPointNDC( const LVector3& Point, const LMatrix4& Projection, const LMatrix4& ModelView );
		scriptexport( Math )  LMatrix4    ObliqueReflectionProjection( const LMatrix4& Projection, const LVector4& ClipPlane );

		/// look at the center of a AABB from such a distance (closest) that the box is entirely visible
		scriptexport( Math )  LMatrix4    GetClosestViewForAABB( const LVector3& MinV, const LVector3& MaxV, const LMatrix4& Proj, const LVector3& Eye, const LVector3& Up );

		scriptexport( Math )  void        FrustumToParams( const LMatrix4& M, float& L, float& R, float& B, float& T, float& N, float& F );
		scriptexport( Math )  void        PerspectiveToParams( const LMatrix4& M, float& FOV, float& Aspect, float& NearCP, float& FarCP );
		scriptexport( Math )  void        OrthoToParams( const LMatrix4& M, float& L, float& R, float& B, float& T, float& N, float& F );

		scriptexport( Math )  bool        IsOrthographicProjection( const LMatrix4& M );
		scriptexport( Math )  bool        IsPerspectiveProjection( const LMatrix4& M );

		scriptexport( Math )  void TransformRayToCoordinates( const LVector3& P, const LVector3& A, const LMatrix4& Transform, LVector3& TransP, LVector3& TransA );

		LProjectionType DetermineProjectionType( const LMatrix4& Projection );
	};
};

/// Utility class representing the 3D transformation
class LTransform
{
public:
	LTransform();
	LTransform( const LVector3& pos, const LQuaternion& quat );
	LTransform( const LMatrix4& mtx4 );

	void SetPositionAndAngles( const LVector3& Pos, float AngleX, float AngleY, float AngleZ );
	void SetPositionAndAngles( const LVector3& Pos, const LVector3& Angles );
	void SetPositionAndAxisAngle( const LVector3& Pos, const LVector3& Axis, float Angle );
	void SetPositionAndOrientation( const LVector3& pos, const LQuaternion& quat );
	void SetPosMatrixAndAxisAngle( const LMatrix4& Pos, const LVector3& Axis, float Angle );
	void SetMatrix4( const LMatrix4& Mtx4 )
	{
		FMatrix = Mtx4;
	};

	const LMatrix4& GetMatrix4() const
	{
		return FMatrix;
	};
	void GetPositionAndOrientation( LVector3& Pos, LQuaternion& Q ) const;

	void LookAt( const LVector3& From, const LVector3& To, const LVector3& Up );

	/// Lerp + SLerp between O1 and O2 for t in [0,1]
	void Interpolate( const LTransform& O1, const LTransform& O2, float t );

	scriptmethod void              SetAngleSystem( LEulerAngleSystem AS )
	{
		FAngleSystem = AS;
	}
	scriptmethod LEulerAngleSystem GetAngleSystem() const
	{
		return FAngleSystem;
	}

	void     SetAngles( const LVector3& Angles );
	LVector3 GetAngles() const;

	void     SetAngleTriple( float T1, float T2, float T3 );
	void     GetAngleTriple( float& T1, float& T2, float& T3 ) const;

	virtual void     SetPosition( const LVector3& P );
	virtual LVector3 GetPosition() const;
public:
	LMatrix4 FMatrix;

	/// Currently used 3-angle system for orientation
	LEulerAngleSystem FAngleSystem;
};

scriptexport( Math ) void DecomposeTransformation( const LMatrix4& T, LVector3& Pos, LMatrix4& Rot );
scriptexport( Math ) void DecomposeCameraTransformation( const LMatrix4& T, LVector3& Pos, LMatrix4& Rot );

scriptexport( Math ) LMatrix4 ComposeTransformation( const LVector3& Pos, const LMatrix4& Rot );
scriptexport( Math ) LMatrix4 ComposeCameraTransformation( const LVector3& Pos, const LMatrix4& Rot );

/**
   Calculate three Euler angles from orientation in a given axis system (ZXZ etc.)
*/
void MatrixToAngles( LEulerAngleSystem Sys, const LMatrix3& M, float& T1, float& T2, float& T3 );

/**
   Calculate orientation from three Euler angles in a given axis system (ZXZ etc.)
*/
void AnglesToMatrix( LEulerAngleSystem Sys, LMatrix4& M, float T1, float T2, float T3 );

namespace Linderdaum
{
	namespace Math
	{
		scriptexport( Math ) void     Randomize( int Seed );
		scriptexport( Math ) int      Random( int L );
		scriptexport( Math ) float    Random( float L );
		scriptexport( Math ) float    Random();
		scriptexport( Math ) float    RandomInRange( float RMin, float RMax );
		scriptexport( Math ) LVector3 RandomVector3InRange( const LVector3& RMin,
		                                                    const LVector3& RMax );
		scriptexport( Math ) LVector4 RandomVector4InRange( const LVector4& RMin,
		                                                    const LVector4& RMax );
		scriptexport( Math ) int      RandomInRange( int RMin, int RMax );
	};
};

/// Max noise dimension
const int MAX_DIMENSIONS = 3;

/// Maximum number of octaves in an fBm object
const int MAX_OCTAVES = 128;

/**
 \brief Multidimensional noise generator

 This class implements the Perlin noise function. Initialize it with the number
 of dimensions (1 to 4) and a random seed. I got the source for the first 3
 dimensions from "Texturing & Modeling: A Procedural Approach".
 The noise buffers are set up as member variables so that
 there may be several instances of this class in use at the same time, each
 initialized with different parameters.

 This class also implements fBm, or fractal Brownian motion.
 H (roughness ranging from 0 to 1), and the lacunarity (2.0 is often used).

 Taken from source code which is Copyright (c) 2000, Sean O'Neil (s_p_oneil@hotmail.com, http://sponeil.net/)
 All rights reserved.  (BSD/MIT-like license)
*/
class LNoise
{
public:

	LNoise() {}
	LNoise( int nDimensions, unsigned int nSeed ) { InitNoise( nDimensions, nSeed ); }

	LNoise( int nDimensions, unsigned int nSeed, float H, float Lacunarity )
	{
		InitNoise( nDimensions, nSeed );
		InitFractal( H, Lacunarity );
	}

	/// Initialize internal random generator and tables
	void InitNoise( int nDimensions, unsigned int nSeed );

	/// Initialize internal fractal parameters
	void InitFractal( float H, float Lacunarity )
	{
		FH = H;
		FLacunarity = Lacunarity;

		float f = 1;

		for ( int i = 0 ; i < MAX_OCTAVES ; i++ )
		{
			FExponent[i] = powf( f, -H );
			f *= Lacunarity;
		}
	}

	/// Random noise
	float Noise( float* f );

	/// Fractal brownian motion
	float fBm( float* f, float Octaves );

protected:

	#pragma region Noise parameters

	/// Number of dimensions used by this object
	int    FDimensions;

	/// Randomized map of indexes into buffer
	unsigned char FMap[256];

	/// Random n-dimensional buffer
	float  FBuffer[256][MAX_DIMENSIONS];

	#pragma endregion

	#pragma region Fractal properties

	/// Roughness
	float FH;

	/// Lacunarity parameter
	float FLacunarity;

	/// Precalculated exponents
	float FExponent[MAX_OCTAVES];

	#pragma endregion

	/// Internal noise function
	float Lattice( int ix, float fx, int iy = 0, float fy = 0, int iz = 0, float fz = 0, int iw = 0, float fw = 0 )
	{
		int   n[4] = {ix, iy, iz, iw};
		float f[4] = {fx, fy, fz, fw};

		int i, Idx = 0;

		for ( i = 0; i < FDimensions ; i++ )
		{
			Idx = FMap[( Idx + n[i] ) & 0xFF];
		}

		float Value = 0;

		for ( i = 0 ; i < FDimensions ; i++ )
		{
			Value += FBuffer[Idx][i] * f[i];
		}

		return Value;
	}
};

namespace Linderdaum
{
	/// Transformation of colors between different representation formats
	namespace LColors
	{
		inline LVector4 BGRA8toRGBAvec4( const unsigned int* Color )
		{
			unsigned char* BC = ( unsigned char* )Color;
			return LVector4( static_cast<float>( BC[2] ) / 255.0f,
			                 static_cast<float>( BC[1] ) / 255.0f,
			                 static_cast<float>( BC[0] ) / 255.0f,
			                 static_cast<float>( BC[3] ) / 255.0f );
		};

		inline LVector4 RGBA8toRGBAvec4( const unsigned int* Color )
		{
			unsigned char* BC = ( unsigned char* )Color;
			return LVector4( static_cast<float>( BC[0] ) / 255.0f,
			                 static_cast<float>( BC[1] ) / 255.0f,
			                 static_cast<float>( BC[2] ) / 255.0f,
			                 static_cast<float>( BC[3] ) / 255.0f );
		};

		/// helper to convert hexadecimal RGB/RGBA-colors (like 0x6683a3) into vec4, alpha is set to 1.0 for values 0xFFFFFF and lower
		/*inline LVector4 ToColor( uint64_t Color )
		{
		   return ( Color > 0xFFFFFF ) ? LVector4( float( ( Color >> 24 ) & 0xFF ) / 255.0f,
		                                           float( ( Color >> 16 ) & 0xFF ) / 255.0f,
		                                           float( ( Color >> 8  ) & 0xFF ) / 255.0f,
		                                           float( ( Color >> 0  ) & 0xFF ) / 255.0f ) :
		          LVector4( float( ( Color >> 16 ) & 0xFF ) / 255.0f,
		                    float( ( Color >> 8  ) & 0xFF ) / 255.0f,
		                    float( ( Color >> 0  ) & 0xFF ) / 255.0f, 1.0f );
		}*/
	}; // LColors

}; // Linderdaum

/// 2D rectangle (usually represent a 2D screen area)
class LRect
{
public:
	LRect() : FExtents() {};
	LRect( const LRect& Rect ) : FExtents( Rect.FExtents ) {};
	explicit LRect( float Value ) : FExtents( Value ) {};
	explicit LRect( const LVector4& Extents ) : FExtents( Extents ) {};
	LRect( float X1, float Y1 ) : FExtents( X1, Y1, 1.0f, 1.0f ) {};
	LRect( float X1, float Y1, float X2, float Y2 ) : FExtents( X1, Y1, X2, Y2 ) {};
	LRect( const LVector2& TL, const LVector2& BR ) : FExtents( TL.x, TL.y, BR.x, BR.y ) {};
	LRect( int X1, int Y1, int X2, int Y2 ) : FExtents( static_cast<float>( X1 ), static_cast<float>( Y1 ), static_cast<float>( X2 ), static_cast<float>( Y2 ) ) {};

	inline float X1() const { return FExtents.x; };
	inline float Y1() const { return FExtents.y; };
	inline float X2() const { return FExtents.z; };
	inline float Y2() const { return FExtents.w; };
	inline float& X1() { return FExtents.x; };
	inline float& Y1() { return FExtents.y; };
	inline float& X2() { return FExtents.z; };
	inline float& Y2() { return FExtents.w; };

	inline float    GetWidth() const  { return fabs( FExtents.z - FExtents.x ); };
	inline float    GetHeight() const { return fabs( FExtents.w - FExtents.y ); };

	inline void     SetWidth( float Width ) { FExtents.z = FExtents.x + Width; };
	inline void     SetHeight( float Height ) { FExtents.w = FExtents.y + Height; };

	inline float    GetCenterX() const { return ( FExtents.x + FExtents.z ) * 0.5f; };
	inline float    GetCenterY() const { return ( FExtents.y + FExtents.w ) * 0.5f; };
	inline LVector2 GetCenter() const { return LVector2( GetCenterX(), GetCenterY() ); };
	inline LVector2 GetTopLeft() const { return LVector2( FExtents.x, FExtents.y ); };
	inline LVector2 GetTopRight() const { return LVector2( FExtents.z, FExtents.y ); };
	inline LVector2 GetBottomLeft() const { return LVector2( FExtents.x, FExtents.w ); };
	inline LVector2 GetBottomRight() const { return LVector2( FExtents.z, FExtents.w ); };

	/// fit Other rect inside this rect respecting aspect ratio of Other
	LRect Fit( const LRect& Other )
	{
		float OldWidth  = Other.GetWidth();
		float OldHeight = Other.GetHeight();

		float Aspect1 = OldWidth  / GetWidth();
		float Aspect2 = OldHeight / GetHeight();

		float Aspect = Linderdaum::Math::LMax( Aspect1, Aspect2 );

		LRect Result( Other );

		Result.SetWidth(  OldWidth  / Aspect );
		Result.SetHeight( OldHeight / Aspect );

		return Result;
	}

	/// center Other rect inside this rect
	LRect Center( const LRect& Other )
	{
		float W = Other.GetWidth();
		float H = Other.GetHeight();

		LVector2 C = GetCenter();

		float X = C.x - W / 2;
		float Y = C.y - H / 2;

		return LRect( X, Y, X + W, Y + H );
	}

	/// construct a LRect enclosing this and Other assuming FixOrder() has been already called on both
	void    Combine( const LRect& Other )
	{
		FExtents.x = Linderdaum::Math::LMin( FExtents.x, Other.FExtents.x );
		FExtents.z = Linderdaum::Math::LMax( FExtents.z, Other.FExtents.z );

		FExtents.y = Linderdaum::Math::LMin( FExtents.y, Other.FExtents.y );
		FExtents.w = Linderdaum::Math::LMax( FExtents.w, Other.FExtents.w );
	}

	void Remap( int Width, int Height )
	{
		float W = 1.0f / Width;
		float H = 1.0f / Height;

		float dW = 0.5f * W;
		float dH = 0.5f * H;

		FExtents = LVector4( X1() * W + dW, Y1() * H - dH, X2() * W + dW, Y2() * H + dH );
	}

	inline void    MoveTo( const LVector2& LeftTop )
	{
		/*
		      float W = GetWidth();
		      float H = GetHeight();

		      FExtents.X = LeftTop.X;
		      FExtents.Y = LeftTop.Y;

		      FExtents.Z = FExtents.X + W;
		      FExtents.W = FExtents.Y + H;
		*/
		FExtents = LVector4( LeftTop.x, LeftTop.y, LeftTop.x + GetWidth(), LeftTop.y + GetHeight() );
	}

	inline void    MoveRel( const LVector2& Delta ) { FExtents += LVector4( Delta.x, Delta.y, Delta.x, Delta.y ); }

	/// Check if the Point is inside this rectangle
	inline bool    ContainsPoint( const LVector2& Point ) const
	{
		return Point.x >= FExtents.x && Point.y >= FExtents.y && Point.x <= FExtents.z && Point.y <= FExtents.w;
	}

	/// Check if R overlaps this rectangle
	inline bool    Overlap( const LRect& R ) const
	{
		return !( X1() > R.X2() || R.X1() > X2() || Y1() > R.Y2() || R.Y1() > Y2() );
	}

	/// try to align coords (X, Y) to the coords of this rect
	void    DockCoordsToThisRect( float* X, float* Y, const float W, const float H, float DockingDistance ) const
	{
		if ( fabsf( *X - FExtents.x ) < DockingDistance ) { *X = FExtents.x; }

		if ( fabsf( *Y - FExtents.y ) < DockingDistance ) { *Y = FExtents.y; }

		if ( fabsf( *X - FExtents.z ) < DockingDistance ) { *X = FExtents.z; }

		if ( fabsf( *Y - FExtents.w ) < DockingDistance ) { *Y = FExtents.w; }

		if ( fabsf( *X + W - FExtents.x ) < DockingDistance ) { *X = FExtents.x - W; }

		if ( fabsf( *Y + H - FExtents.y ) < DockingDistance ) { *Y = FExtents.y - H; }

		if ( fabsf( *X + W - FExtents.z ) < DockingDistance ) { *X = FExtents.z - W; }

		if ( fabsf( *Y + H - FExtents.w ) < DockingDistance ) { *Y = FExtents.w - H; }
	}

	/// try to align size (W, H) to the coords of this rect
	void    DockSizeToThisRect( const float X, const float Y, float* W, float* H, float DockingDistance ) const
	{
		if ( fabsf( X + *W - FExtents.x ) < DockingDistance ) { *W = FExtents.x - X; }

		if ( fabsf( Y + *H - FExtents.y ) < DockingDistance ) { *H = FExtents.y - Y; }

		if ( fabsf( X + *W - FExtents.z ) < DockingDistance ) { *W = FExtents.z - X; }

		if ( fabsf( Y + *H - FExtents.w ) < DockingDistance ) { *H = FExtents.w - Y; }
	}

	/// ensure X2>=X1 and Y2>=Y1
	void    FixOrder()
	{
		if ( FExtents.x > FExtents.z ) { Linderdaum::Math::SwapValues( FExtents.x, FExtents.z ); }

		if ( FExtents.y > FExtents.w ) { Linderdaum::Math::SwapValues( FExtents.y, FExtents.w ); }
	}

	inline const LVector4& ToVector4() const { return FExtents; };
	inline       LVector4& ToVector4()       { return FExtents; };
	inline const float* ToFloatPtr() const { return FExtents.ToFloatPtr(); };
	inline float*       ToFloatPtr()       { return FExtents.ToFloatPtr(); };

	/// the resulting rect lays within this rect and has the aspect ratio Aspect
	LRect GetAdjustedAspectRect( float Aspect ) const
	{
		float W = GetWidth();
		float H = GetHeight();

		if ( W / H > Aspect )
		{
			float Wt = W * Aspect;

			return LRect( X1() + 0.5f * ( W - Wt ), Y1(), X1() + 0.5f * ( W + Wt ), Y1() + H );
		}

		float Ht = H / Aspect;
		return LRect( X1(), Y1() + 0.5f * ( H - Ht ), X1() + W, Y1() + 0.5f * ( H + Ht ) );
	}
public:
	/// X:X1, Y:Y1, Z:X2, W:Y2
	LVector4 FExtents;
};
