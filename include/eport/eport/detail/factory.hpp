

#pragma once

#include <memory>

/***********************************************************************************/

template <typename Class> class shared_class
	: public std::enable_shared_from_this<Class> {

public:
	typedef std::shared_ptr<Class> ref;
	static inline ref create() {
		struct T : public Class { T() : Class() {} };
		return std::make_shared<T>();
	}

	template <typename... Args>
	static inline ref create(Args... args) {
		struct T : public Class { T(Args... args) : Class(args...) {} };
		return std::make_shared<T>(args...);
	}
};

/***********************************************************************************/
