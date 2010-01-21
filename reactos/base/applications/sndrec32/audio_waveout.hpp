#ifndef _AUDIOWAVEOUT__H_
#define _AUDIOWAVEOUT__H_


#include "audio_def.hpp"
#include "audio_format.hpp"
#include "audio_producer.hpp"



_AUDIO_NAMESPACE_START_





enum audio_waveout_status { WAVEOUT_NOTREADY, WAVEOUT_READY,
						    WAVEOUT_PLAYING, WAVEOUT_ERR,
						    WAVEOUT_PAUSED, WAVEOUT_STOP

						   };





class audio_waveout
{

	friend class audio_buffer;



	private:


		static DWORD WINAPI playing_procedure( LPVOID );



		HANDLE wakeup_playthread;




	protected:


		WAVEFORMATEX   wave_format;
		WAVEHDR        * wave_headers;
		HWAVEOUT       waveout_handle;





		const audio_format & aud_info;
		audio_producer & audio_buf;






		//
		// Audio Playing Thread id
		//

		DWORD     playthread_id;





		audio_waveout_status status;




		float buf_secs;



		//
		// The temporary buffers for the audio
		// data outgoing to the waveout device
		// and its size, and its total number.
		//


		//base address for entire memory
		BYTE * main_buffer;

		//size in bytes for the entire memory
		unsigned int mb_size;

		//number of little buffers
		unsigned int buffers;






		//
		// Protected Functions
		//

		void init_( void );
		void alloc_buffers_mem_( unsigned int, float );
		void free_buffers_mem_( void );


		void init_headers_( void );
		void prep_headers_( void );
		void unprep_headers_( void );







	public:

		//
		// Ctors
		//

		audio_waveout( const audio_format & aud_fmt,
								audio_producer & a_buf )

			: wave_headers( 0 ), aud_info( aud_fmt ),
			audio_buf( a_buf ),  status( WAVEOUT_NOTREADY ),
			main_buffer( 0 ), mb_size( 0 ),
			buffers( _AUDIO_DEFAULT_WAVEOUTBUFFERS )
		{

			//
			// Initializing internal wavein data
			//


			init_();

		}






		//
		// Dtor
		//

		~audio_waveout( void )
		{  }




		//
		// Public Functions
		//

		void open  ( void );
		void play  ( void );
		void pause ( void );
		void stop  ( void );
		void close ( void );


		audio_waveout_status current_status( void )
		{ return status; }








};



_AUDIO_NAMESPACE_END_




#endif //ifdef _AUDIOWAVEOUT__H_
