#pragma once
#include <vector>
#include <string>

class VectorField
{
	using elem_t = std::pair<int, int>;
private:
	std::vector<std::vector<elem_t>> field;
	std::vector<std::vector<elem_t>> transformField;

public:
	static VectorField* loadFromFile(std::string const& filename);
	static std::vector<VectorField*> loadAllFromDir(std::string const& path);

	static VectorField* customField(size_t width, size_t height);

public:
	VectorField(size_t x, size_t y);
	~VectorField();

	unsigned char* apply_field(unsigned char const* imageData, size_t x, size_t y, size_t n) const;
	float* raw_data() const;

	void invert();
	void inv();

	void setCycleField();
	void setHalfSpeedTransformField(int speed);
	void setDiagField(size_t turnsNum = 1);
	void setCircleField();
	void setSinField();
	void setField();

	void AddDots1();
	void AddDots2();
	void AddDots3();
	void AddDots4();

	void setSnakeField();
};
