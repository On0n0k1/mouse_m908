/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

//main class
#ifndef RD_MOUSE
#define RD_MOUSE

#include <libusb.h>

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string>
#include <utility>
#include <variant>

/* These declarations exist to make it possible for mouse_variant
 * to use these classes.
 */
class mouse_generic;
class mouse_m607;
class mouse_m709;
class mouse_m711;
class mouse_m715;
class mouse_m719;
class mouse_m721;
class mouse_m908;
class mouse_m913;
class mouse_m990;
class mouse_m990chroma;

/// Calls the function fn with an object of each type in the variant V
template< typename V, size_t I = std::variant_size_v<V>-1, typename F > void variant_loop(F fn){

	V var = V(std::in_place_index<I>);
	std::visit( fn, var );

	if constexpr ( I > 0 ){
		variant_loop< V, I-1, F >(fn);
	}
}

/**
 * This class is used as a base for the different models
 * 
 * Global constants (e.g. keyboard keycodes) and universal functions
 * should be placed in this class.
 * 
 */
class rd_mouse{
	
	public:
		
		/** \brief This struct acts as a default value for mouse_variant.
	 	 * Using std::monostate is not possible because the get_vid(), get_pid() and get_name() functions are not defined but used for detection.
		 * \see rd_mouse::mouse_variant
		 */
		struct monostate{
			static std::string get_name(){ return ""; }
			static void set_vid( uint16_t vid ){ (void)vid; }
			static void set_pid( uint16_t pid ){ (void)pid; }
			static bool has_vid_pid( uint16_t vid, uint16_t pid ){
				(void)vid;
				(void)pid;
				return false;
			}
		};

		// enums
		/// The available profiles
		enum rd_profile{
			profile_1 = 0,
			profile_2 = 1,
			profile_3 = 2,
			profile_4 = 3,
			profile_5 = 4,
		};
		
		/// The available led modes
		enum rd_lightmode{
			lightmode_breathing,
			lightmode_rainbow,
			lightmode_static,
			lightmode_wave,
			lightmode_alternating,
			lightmode_reactive,
			lightmode_flashing,
			lightmode_off,
			lightmode_random,
			lightmode_reactive_button,
			lightmode_breathing_rainbow
		};
		
		/// The available USB report rates (polling rates)
		enum rd_report_rate{
			r_125Hz,
			r_250Hz,
			r_500Hz,
			r_1000Hz
		};

		/// This variant can hold an object for all available mice
		typedef std::variant<
			rd_mouse::monostate,
			mouse_m607,
			mouse_m709,
			mouse_m711,
			mouse_m715,
			mouse_m719,
			mouse_m721,
			mouse_m908,
			mouse_m913,
			mouse_m990,
			mouse_m990chroma,
			mouse_generic // needs to be last to take the lowest priority during detection
		> mouse_variant;
		
		/** \brief Detects supported mice
		 * In the case of multiple connected mice, only the first will be detected
		 * \return A mouse_variant containing an object corresponding to the detected mouse, or rd_mouse::monostate
		 */
		static mouse_variant detect();

		/** \brief Detects supported mice that have a specified name
		 * \arg mouse_name detects mice with name = mouse_name
		 * In the case of multiple connected mice, only the first will be detected
		 * \return A mouse_variant containing an object corresponding to the detected mouse, or rd_mouse::monostate
		 */
		static mouse_variant detect( const std::string& mouse_name );
		
		/// Set whether to try to detach the kernel driver when opening the mouse
		void set_detach_kernel_driver( bool detach_kernel_driver ){
			_i_detach_kernel_driver = detach_kernel_driver;
		}
		/// Get _i_detach_kernel_driver
		bool get_detach_kernel_driver(){ return _i_detach_kernel_driver; }
		
		/// Returns a reference to _c_lightmode_strings (lighmode names)
		std::map< rd_mouse::rd_lightmode, std::string >& lightmode_strings(){ return _c_lightmode_strings; }
		/// Returns a reference to _c_report_rate_strings (report rate names)
		std::map< rd_mouse::rd_report_rate, std::string >& report_rate_strings(){ return _c_report_rate_strings; }

	protected:
		
		//setting min and max values
		static const uint8_t _c_scrollspeed_min, _c_scrollspeed_max;
		static const uint8_t _c_brightness_min, _c_brightness_max;
		static const uint8_t _c_speed_min, _c_speed_max;
		static const uint8_t _c_level_min, _c_level_max;
		static const uint8_t _c_dpi_min, _c_dpi_max;
		static const uint8_t _c_dpi_2_min, _c_dpi_2_max;
		
		//mapping of button names to values
		/// Values/keycodes of mouse buttons and special button functions
		static std::map< std::string, std::array<uint8_t, 4> > _c_keycodes;
		/// Values of keyboard modifiers
		static const std::map< std::string, uint8_t > _c_keyboard_modifier_values;
		/// Values/keycodes of keyboard keys
		static std::map< std::string, uint8_t > _c_keyboard_key_values;
		/// DPI values for the snipe button
		static std::map< int, uint8_t > _c_snipe_dpi_values;
		/// Bytecode for the poll/report rate
		static std::map< uint8_t, rd_mouse::rd_report_rate > _c_report_rate_values;
		/// String representations for the poll/report rate
		static std::map< rd_mouse::rd_report_rate, std::string > _c_report_rate_strings;
		/// Bytecode for the lightmode
		static std::map< std::array<uint8_t, 2>, rd_mouse::rd_lightmode > _c_lightmode_values;
		/// String representations for the lightmode
		static std::map< rd_mouse::rd_lightmode, std::string > _c_lightmode_strings;

		//usb device handling
		/// libusb device handle
		libusb_device_handle* _i_handle = nullptr;
		/// whether to detach kernel driver
		bool _i_detach_kernel_driver = true;
		/// set by open_mouse for close_mouse
		bool _i_detached_driver_0 = false;
		/// set by open_mouse for close_mouse
		bool _i_detached_driver_1 = false;
		/// set by open_mouse for close_mouse
		bool _i_detached_driver_2 = false;
		
		/** \brief Init libusb and open the mouse by its USB VID and PID
		 * \return 0 if successful
		 */
		int _i_open_mouse( const uint16_t vid, const uint16_t pid );
		
		/** \brief Init libusb and open the mouse by the USB bus and device adress
		 * \return 0 if successful
		 */
		int _i_open_mouse_bus_device( const uint8_t bus, const uint8_t device );
		
		/** \brief Close the mouse and libusb
		 * \return 0 if successful (always at the moment)
		 */
		int _i_close_mouse();
		
		
		// bytecode/string conversion functions TODO! add missing functions
		/** \brief Decode macro byte code (of one macro) and print the commands to output
		 * \arg macro_bytes the macro bytecode
		 * \arg output where to print to
		 * \arg prefix string printed before each action
		 * \arg offset start decoding from macro_bytes[offset], set to 0 if offset >= macro_bytes.size()
		 * \return 0 if no invalid codes were encountered
		 */
		static int _i_decode_macro( const std::vector< uint8_t >& macro_bytes, std::ostream& output, const std::string& prefix, size_t offset );
		
		/** \brief Encode macro commands to macro bytecode
		 * \arg macro_bytes holds the result
		 * \arg input where the macro commands are read from
		 * \arg offset skips offset bytes at the beginning
		 */
		static int _i_encode_macro( std::array< uint8_t, 256 >& macro_bytes, std::istream& input, const size_t offset );
		
		/** \brief Decodes the bytes describing a button mapping
		 * \arg bytes the 4 bytes descriping the mapping
		 * \arg mapping string to hold the result
		 * \return 0 if valid button mapping
		 * \see _i_encode_button_mapping
		 */
		static int _i_decode_button_mapping( const std::array<uint8_t, 4>& bytes, std::string& mapping );
		
		/** \brief Turns a string describing a button mapping into bytecode
		 * \arg mapping button mapping
		 * \arg bytes holds the result
		 * \return 0 if valid button mapping
		 * \see _i_decode_button_mapping
		 */
		static int _i_encode_button_mapping( const std::string& mapping, std::array<uint8_t, 4>& bytes );
		
		/** Convert raw dpi bytes to a string representation (doesn't validate dpi value)
		 * This implementation always outputs the raw bytes as a hexdump,
		 * to support actual DPI values this function needs to be overloaded in the model specific classes.
		 * \return 0 if no error occured
		 */
		static int _i_decode_dpi( const std::array<uint8_t, 2>& dpi_bytes, std::string& dpi_string );
		
		/** Convert the bytecode for a lightmode to a string
		 * \return 0 if the lightmode is valid
		 */
		static int _i_decode_lightmode( const std::array<uint8_t, 2>& lightmode_bytes, std::string& lightmode_string );

		/** Convert a lightmode to bytecode
		 * \return 0 if the lightmode is valid
		 */
		static int _i_encode_lightmode( const rd_mouse::rd_lightmode lightmode, std::array<uint8_t, 2>& lightmode_bytes );

		/** Convert the bytecode for a USB report/poll rate to a string
		 * \return 0 if the report rate is valid
		 */
		static int _i_decode_report_rate( const uint8_t report_rate_byte, std::string& report_rate_string );

		/** Convert a report rate to bytecode
		 * \return the byte representing the given report rate
		 */
		static uint8_t _i_encode_report_rate( const rd_mouse::rd_report_rate report_rate );
};

#endif

// include header files for the individual models
#include "m607/mouse_m607.h"
#include "m709/mouse_m709.h"
#include "m711/mouse_m711.h"
#include "m715/mouse_m715.h"
#include "m719/mouse_m719.h"
#include "m721/mouse_m721.h"
#include "m908/mouse_m908.h"
#include "m913/mouse_m913.h"
#include "m990/mouse_m990.h"
#include "m990chroma/mouse_m990chroma.h"
#include "generic/mouse_generic.h"
