#include <limits>
#include <vector>

template<typename T>
struct slot_map {
	using index_type = typename std::vector<T>::size_type;
	using iterator = typename std::vector<T>::iterator;
	using const_iterator = typename std::vector<T>::const_iterator;
	using value_type = T;
	using reference = T & ;
	using const_reference = const T&;
	using pointer = T * ;
	using const_pointer = const T*;
	using differnce_type = typename std::vector<T>::difference_type;
	using size_type = typename std::vector<T>::size_type;


	struct handle {
		handle() = default;

	private:
		handle(slot_map::index_type index_,
			unsigned long generation_) : index_(index_),
			generation_(generation_) {}

		friend struct slot_map<T>;
		index_type index_ = 0;
		unsigned long generation_ = 0;
	};

	void swap(slot_map &arg) {
		using std::swap;
		swap(data_, arg.data_);
		swap(indices_, arg.indices_);
		swap(erase_, arg.erase_);
		freeHead_ = arg.freeHead_;
		freeTail_ = arg.freeTail_;
	}

	handle insert(const T &v) {
		if (freeHead_ == freeTail_) {
			extendIndices();
		}
		data_.push_back(v);
		erase_.push_back(freeHead_);
		handle &currentIndexSlot = indicesInsert();
		return { erase_.back(), currentIndexSlot.generation_ };
	}

	bool valid(handle h) const {
		return h.index_ < indices_.size() &&
			indices_[h.index_].generation_ == h.generation_;
	};

	void erase(handle h) {
		using std::swap;

		if (!valid(h) || !get(h)) {
			return;
		}
		indices_[h.index_].generation_++;
		index_type dataIndex = indices_[h.index_].index_;
		index_type dataLastIndex = data_.size() - 1;

		swap(data_[dataIndex], data_[dataLastIndex]);
		data_.pop_back();
		swap(erase_[dataIndex], erase_[dataLastIndex]);
		erase_.pop_back();

		if (!data_.empty()) {
			indices_[erase_[dataIndex]].index_ = dataIndex;
		}
		indices_[freeTail_].index_ = h.index_;
		freeTail_ = h.index_;
	};

	reference operator[](handle h) {
		return data_[indices_[h.index_].index_];
	};

	const_reference operator[](handle h) const {
		return data_[indices_[h.index_].index_];
	};

	bool operator==(const slot_map<T> &arg) const {
		return data_ == arg.data_;
	}

	bool operator!=(const slot_map<T> &arg) const {
		return data_ != arg.data_;
	}

	pointer get(handle h) {
		return valid(h) ? &(*this)[h] : nullptr;
	};

	const_pointer get(handle h) const {
		return valid(h) ? &(*this)[h] : nullptr;
	};

	size_type size() const { return data_.size(); }

	size_type max_size() const {
		return std::numeric_limits<size_type>::max();
	}

	bool empty() const { return data_.empty(); }

	iterator begin() { return data_.begin(); }

	iterator end() { return data_.end(); }

	const_iterator begin() const { return data_.begin(); }

	const_iterator end() const { return data_.end(); }

	const_iterator cbegin() const { return data_.cbegin(); }

	const_iterator cend() const { return data_.cend(); }

	void reserve(size_t amount) {
		data_.reserve(amount); indices_.reserve(amount); erase_.reserve(amount);
	}
private:
	std::vector<T> data_;
	std::vector<handle> indices_{ {} };
	std::vector<index_type> erase_;

	index_type freeHead_ = 0;
	index_type freeTail_ = 0;

	void extendIndices() {
		indices_.push_back(handle());
		freeTail_ = indices_.size() - 1;
		indices_[freeHead_].index_ = freeTail_;
	}

	handle &indicesInsert() {
		handle &currentSlot = indices_[freeHead_];
		if (freeHead_ != freeTail_) {
			freeHead_ = indices_[freeHead_].index_;
		}
		currentSlot.index_ = data_.size() - 1;
		return currentSlot;
	}
};


template<class T>
void swap(slot_map<T> &lhs, slot_map<T> &arg) {
	lhs.swap(arg);
}