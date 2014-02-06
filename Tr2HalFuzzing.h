#pragma once
#ifndef Tr2HalFuzzing_h_
#define Tr2HalFuzzing_h_

#if !defined( NDEBUG )

#	define	AL_FUZZ( objectType )														\
		{																				\
			extern float alFuzzValue	[ Tr2RenderContextEnum::OBJECT_TYPE_COUNT ];	\
			if( alFuzzValue[ objectType ] &&											\
				rand() / float(RAND_MAX) < alFuzzValue[ objectType ] )					\
			{																			\
				return E_FAIL;															\
			}																			\
		}

#	define	AL_FUZZ_RET( objectType, returnValue )										\
		{																				\
			extern float alFuzzValue	[ Tr2RenderContextEnum::OBJECT_TYPE_COUNT ];	\
			if( alFuzzValue[ objectType ] &&											\
				rand() / float(RAND_MAX) < alFuzzValue[ objectType ] )					\
			{																			\
				return returnValue;														\
			}																			\
		}

#	define	AL_FUZZ_LOCK( objectType )													\
		{																				\
			extern float alFuzzLockValue[ Tr2RenderContextEnum::OBJECT_TYPE_COUNT ];	\
			if( alFuzzLockValue[ objectType ] &&										\
				rand() / float(RAND_MAX) < alFuzzLockValue[ objectType ] )				\
			{																			\
				return E_FAIL;															\
			}																			\
		}

#else

#	define	AL_FUZZ( objectType )
#	define	AL_FUZZ_RET( objectType, returnValue )
#	define	AL_FUZZ_LOCK( objectType )

#endif 

#endif //Tr2HalFuzzing_h_
