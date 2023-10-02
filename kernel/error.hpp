#pragma once

#include <array>

class Error {
public:
	enum Code {
		kSuccess,
		kFull,
		kEmpty,
		kLastOfCode,
	};

	Error(Code code) : code_(code) {}
	const char* Name() const {
		return code_names_[static_cast<int>(this->code_)];
	}

	operator bool() const {
		return this->code_ != kSuccess;	// エラーが起きていたら true
	}

private:
	static constexpr std::array<const char*, 3>	code_names_ = {
		"kSuccess",
		"kFull",
		"kEmpty",
	};
	Code	code_;
};
