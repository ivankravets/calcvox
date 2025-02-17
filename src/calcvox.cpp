#include <Arduino.h>
#include <Keypad.h>
#include "AudioTools.h"
#include "filesystems.h"
#include "espeak.h"
#include "SPI.h"
#include <string>
#include <vector>
#include <map>
#include "tinyexpr.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#define CALCVOX_H1
#include "pins.h"

void setup_keypad() {
	for (byte r = 0; r < COLUMNS; r++) {
		pinMode(cols[r], OUTPUT);
		digitalWrite(cols[r], HIGH);
	}
	for (byte c = 0; c < ROWS; c++) {
		pinMode(rows[c], INPUT_PULLUP);
	}
}

std::string get_key() {
	for (byte r = 0; r < COLUMNS; r++) {
		digitalWrite(cols[r], LOW);
		for (byte c = 0; c < ROWS; c++) {
			if (digitalRead(rows[c]) == LOW) {
				delay(10);
				if (digitalRead(rows[c]) == LOW) {
					digitalWrite(cols[r], HIGH);
					Serial.println(keys[r][c].c_str());
					return keys[r][c];
				}
			}
			//delay(5);
		}
		digitalWrite(cols[r], HIGH);
	}
	return "";
}

std::string convert_character(const std::string character) {
	static const std::map<std::string, std::string> char_map = {
		{"0", "0"},
		{"1", "1"},
		{"2", "2"},
		{"3", "3"},
		{"4", "4"},
		{"5", "5"},
		{"6", "6"},
		{"7", "7"},
		{"8", "8"},
		{"9", "9"},
		{"+", "plus"},
		{"-", "minus"},
		{"*", "times"},
		{"/", "divide"},
		{".", "point"},
		{"=", "equals"}
	};
	auto it = char_map.find(character);
	if (it == char_map.end()) {
		return "";
	}
	return it->second;
}

std::string eval(const std::string& expression, const int precision) {
	te_variable vars[] = {};
	int err;
	te_expr* expr = te_compile(expression.c_str(), vars, 0, &err);
	if (!expr) {
		return "Error";
	}
	double result = te_eval(expr);
	te_free(expr);
	std::ostringstream result_stream;
	result_stream << std::fixed << std::setprecision(precision) << result;
	return result_stream.str();
}

#if defined(USE_I2S)
I2SStream audio_output;
#elif defined(USE_ANALOG)
AnalogAudioStream audio_output;
#endif
ESpeak espeak(audio_output);
std::string current_equation;
std::vector<std::string> history;

void setup() {
	Serial.begin(115200);
	espeak.begin();
	auto espeak_info = espeak.audioInfo();
	auto cfg = audio_output.defaultConfig();
	cfg.channels = espeak_info.channels; // 1
	cfg.sample_rate = espeak_info.sample_rate; // 22050
	cfg.bits_per_sample = espeak_info.bits_per_sample; // 16
	#if defined(USE_I2s)
		cfg.i2s_format = I2S_LSB_FORMAT;
		cfg.pin_ws = i2s_ws;
		cfg.pin_bck = i2s_bclk;
		cfg.pin_data = i2s_din;
	#endif
	audio_output.begin(cfg);
	setup_keypad();
}

void loop() {
	std::string key = get_key();
	Serial.println("loop :)");
	if (key != "") {
		if (key == "=") {
			const char* result = eval(current_equation, 2).c_str();
			espeak.say(result);
		} else if (key == "all_clear") {
			if (!current_equation.empty()) {
				current_equation = "";
				espeak.say("All clear");
			} else {
				espeak.say("Empty");
			}
		} else if (key == "delete") {
			if (!current_equation.empty()) {
				std::string last_char = convert_character(current_equation.substr(current_equation.length() - 1));
				current_equation.pop_back();
				espeak.say(last_char.c_str());
			} else {
				espeak.say("Empty");
			}
		} else {
			const char *to_speak = convert_character(key).c_str();
			espeak.say(to_speak);
			current_equation += key;
		}
	}
}
