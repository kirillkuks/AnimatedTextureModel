#include "VectorField.h"
#include <fstream>
#include <assert.h>
#include <filesystem>


VectorField* VectorField::loadFromFile(std::string const& filename)
{
	std::ifstream is(filename);

	if (!is.is_open())
	{
		return nullptr;
	}

	size_t width, height;
	is >> width >> height;

	VectorField* field = new VectorField(width, height);

	if (field == nullptr)
	{
		return nullptr;
	}

	while (!is.eof())
	{
		size_t y1, x1, y2, x2;
		is >> x1 >> y1 >> x2 >> y2;

		int y, x;
		is >> x >> y;

		for (size_t i = x1; i < x2; ++i)
		{
			for (size_t j = y1; j < y2; ++j)
			{
				field->field[i][j].first = x;
				field->field[i][j].second = y;
			}
		}
	}

	return field;
}

// TODO: Конфиг с именами фалов
std::vector<VectorField*> VectorField::loadAllFromDir(std::string const& path)
{
	std::vector<VectorField*> fields;

	/* for (auto const& file : std::filesystem::directory_iterator(path))
	{
		if (file.path().string().find(".fld") != std::string::npos)
		{
			VectorField* field = loadFromFile(file.path().string());

			if (field != nullptr)
			{
				fields.push_back(field);
			}
		}
	} */

	for (size_t i = 1; i <= 11; ++i)
	{
		VectorField* field = loadFromFile(path + "//field" + std::to_string(i) + ".fld");

		if (field != nullptr)
		{
			fields.push_back(field);
		}
	}

	return fields;
}

VectorField* VectorField::customField(size_t width, size_t height)
{
	VectorField* field = new VectorField(width, height);

	for (size_t i = 0; i < width; ++i)
	{
		for (size_t j = 0; j < height; ++j)
		{
			field->field[i][j].first = 20;
			field->field[i][j].second = 0;
		}
	}

	/*for (size_t i = 0; i < width; ++i)
	{
		for (size_t j = 120; j < height; ++j)
		{
			field->field[i][j].first = 0;
			field->field[i][j].second = 100;
		}
	}*/

	return field;
}

VectorField::VectorField(size_t x, size_t y)
	: field(x, std::vector<std::pair<int, int>>(y, { 0, 0 })),
	transformField(x, std::vector<std::pair<int, int>>(y, { 0, 0 }))
{
	// setHalfSpeedTransformField(2);
	// setDiagField(4);
	// setCycleField();
	// int a = 2 + 4;
	// setCircleField();
	// setSinField(); 

	// SetUp 2
	// setField();

	// SetUp 1
	for (size_t i = 0; i < 300; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{
			field[i][j].first = 0;
			field[i][j].second = 0;
		}
	}
}

VectorField::~VectorField() {}

unsigned char* VectorField::apply_field(unsigned char const* imageData, size_t x, size_t y, size_t n) const
{
	size_t size = x * y * n;
	unsigned char* newImageData = new unsigned char[size * sizeof(unsigned char)];

	if (newImageData == nullptr)
	{
		return nullptr;
	}

	std::memcpy(newImageData, imageData, size);

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{
			std::pair<int, int> const& vec = field[i][j];

			size_t n_i = i + vec.first;
			size_t n_j = j + vec.second;

			if (n_i < x && n_j < y)
			{
				size_t n_ind = n * (n_j + n_i * x);
				size_t ind = n * (j + i * x);

				newImageData[n_ind + 0] = imageData[ind + 0];
				newImageData[n_ind + 1] = imageData[ind + 1];
				newImageData[n_ind + 2] = imageData[ind + 2];
			}
		}
	}

	return newImageData;
}

float* VectorField::raw_data() const
{
	size_t x = field.size();
	size_t y = field[0].size();

	size_t n = 4;

	float* srcData = new float[n * x * y];
	assert(srcData);

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j) {
			float q = (float)field[i][j].first / (float)x;
			float p = (float)field[i][j].second / (float)y;

			srcData[n * (i + j * x) + 0] = q;
			srcData[n * (i + j * x) + 1] = p;

			// пока так
			if (p > 0 || p < 0)
			{
				srcData[n * (i + j * x) + 2] = 1.0f;
			}
			else
			{
				srcData[n * (i + j * x) + 2] = 0.0f;
			}

			float transformQ = (float)transformField[i][j].first / (float)x;
			float transformP = (float)transformField[i][j].second / (float)y;

			srcData[n * (i + j * x) + 2] = transformQ;
			srcData[n * (i + j * x) + 3] = transformP;
		}
	}

	return srcData;
}

void VectorField::invert()
{
	size_t x = field.size();
	size_t y = field[0].size();

	std::vector<std::vector<std::pair<int, int>>> newField(x, std::vector<std::pair<int, int>>(y, { 0, 0 }));

	for (int i = 0; i < x; ++i)
	{
		for (int j = 0; j < y; ++j)
		{
			auto& vec = field[i][j];

			int new_i = i + vec.first;
			int new_j = j + vec.second;

			// newField[new_i % x][new_j % y].first = -vec.first;
			// newField[new_i % x][new_j % y].second = -vec.second;

			newField[i][j].first = -vec.first;
			newField[i][j].second = -vec.second;
		}
	}

	field = newField;
}

void VectorField::inv()
{
	size_t x = field.size();
	size_t y = field[0].size();

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{
			elem_t elem = field[i][j];

			field[i + elem.first][j + elem.second].first = -elem.first;
			field[i + elem.first][j + elem.second].second = -elem.second;
		}
	}

}

void VectorField::setCycleField()
{
	size_t x = field.size();
	size_t y = field[0].size();


	for (size_t j = 0; j < y; ++j)
	{
		int speed_x = field[0][j].first;

		for (size_t i = 0; i < speed_x; ++i)
		{
			field[x - i - 1][j] = std::make_pair(-((int)x - speed_x), field[x - i - 1][j].second);
		}
	}
}

void VectorField::setHalfSpeedTransformField(int speed)
{
	size_t x = field.size();
	size_t y = field[0].size();

	size_t half_y = y / 2;

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < half_y; ++j)
		{
			field[i][j] = std::make_pair(speed, 0);
		}
	}
}

void VectorField::setDiagField(size_t turnsNum)
{
	size_t x = field.size();
	size_t y = field[0].size();

	size_t len = (size_t)(x / pow(2, turnsNum));

	int direction = 1;
	size_t p_counter = 0;

	for (size_t i = 0; i < x; ++i)
	{
		if (p_counter == len)
		{
			direction *= -1;
			p_counter = 0;
		}
		++p_counter;

		for (size_t j = 0; j < y; ++j)
		{
			field[i][j].second = 1 * direction;
		}
	}
}

void VectorField::setCircleField()
{
	size_t x = field.size();
	size_t y = field[0].size();

	int centerX = (int)x / 2;
	int centerY = (int)y / 2;

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{

			int newX = (int)i - centerX;
			int newY = (int)j - centerY;

			field[i][j].first = -(int)j;
			field[i][j].second = (int)i;
		}
	}

}

void VectorField::setSinField()
{
	size_t x = field.size();
	size_t y = field[0].size();

	int centerX = (int)x / 2;
	int centerY = (int)y / 2;

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{
			if (i > j && i >= y - j)
			{
				field[i][j].first = 0;
				field[i][j].second = 20;
			}

			if (i >= j && i < y - j)
			{
				field[i][j].first = 20;
				field[i][j].second = 0;
			}

			if (i < j && i <= y - j)
			{
				field[i][j].first = 0;
				field[i][j].second = -20;
			}

			if (i <= j && i > y - j)
			{
				field[i][j].first = -20;
				field[i][j].second = 0;
			}
		}
	}
}

void VectorField::setField()
{
	size_t x = field.size();
	size_t y = field[0].size();

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{
			field[i][j].first = (int)(100 * sin(3.141 * j / (float(y) / 2)));
			field[i][j].second = 0;
		}
	}
}

void VectorField::setSnakeField()
{
	size_t x = field.size();
	size_t y = field[0].size();

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < y; ++j)
		{
			field[i][j].first = 0;
			field[i][j].second = 0;
		}
	}

	for (size_t i = 0; i < x; ++i)
	{
		for (size_t j = 0; j < 290; ++j)
		{
			field[i][j].first = (int)(15 * sin(3.141 * j / (290.0 / 2)));
		}
	}
}

void VectorField::AddDots1()
{
	std::vector<std::pair<int, int>> srcDots;
	std::vector<std::pair<int, int>> dstDots;

	for (int j = 25; j < 45; ++j)
	{
		for (int i = 40; i < 250; ++i)
		{
			srcDots.push_back({ j, i });
		}
	}

	for (int j = 649; j > 629; --j)
	{
		for (int i = 530; i > 320; --i)
		{
			dstDots.push_back({ j, i });
		}
	}

	size_t size = srcDots.size();
	assert(size == dstDots.size());

	for (size_t j = 0; j < size; ++j)
	{
		auto& dst = dstDots[j];
		auto& src = srcDots[j];

		field[dst.first][dst.second] = { src.first - dst.first, src.second - dst.second };
		transformField[dst.first][dst.second] = { src.first - dst.first, src.second - dst.second };
	}
}

void VectorField::AddDots2()
{
	std::vector<std::pair<int, int>> srcDots;
	std::vector<std::pair<int, int>> dstDots;

	/// :(
	for (size_t i = 0; i < field.size(); ++i)
	{
		for (size_t j = 0; j < field[i].size(); ++j)
		{
			transformField[i][j].first = field[i][j].first;
			transformField[i][j].second = field[i][j].second;
		}
	}

	for (int j = 25; j < 55; ++j)
	{
		for (int i = 40; i < 140; ++i)
		{
			srcDots.push_back({ j, i });
		}
	}

	for (int j = 25; j < 55; ++j)
	{
		for (int i = 140; i < 250; ++i)
		{
			srcDots.push_back({ j, i });
		}
	}

	for (int j = 659; j > 629; --j)
	{
		for (int i = 530; i > 430; --i)
		{
			dstDots.push_back({ j, i });
		}
	}

	for (int j = 655; j > 625; --j)
	{
		for (int i = 430; i > 320; --i)
		{
			dstDots.push_back({ j, i });
		}
	}

	size_t size = srcDots.size();
	assert(size == dstDots.size());

	for (size_t j = 0; j < size; ++j)
	{
		auto& dst = dstDots[j];
		auto& src = srcDots[j];

		field[dst.first][dst.second] = { src.first - dst.first, src.second - dst.second };
		transformField[dst.first][dst.second] = { -field[src.first][src.second].first, -field[src.first][src.second].second };
	}

}

void VectorField::AddDots3()
{
	std::vector<std::pair<int, int>> srcDots;
	std::vector<std::pair<int, int>> dstDots;

	for (int j = 25; j < 39; ++j)
	{
		for (int i = 40; i < 250; ++i)
		{
			srcDots.push_back({ j, i });
		}
	}

	for (int j = 644; j > 630; --j)
	{
		for (int i = 530; i > 320; --i)
		{
			dstDots.push_back({ j, i });
		}
	}

	size_t size = srcDots.size();
	assert(size == dstDots.size());

	for (size_t j = 0; j < size; ++j)
	{
		auto& dst = dstDots[j];
		auto& src = srcDots[j];

		field[dst.first][dst.second] = { src.first - dst.first, src.second - dst.second };
		transformField[dst.first][dst.second] = { -field[src.first][src.second].first, -field[src.first][src.second].second };
	}
}

void VectorField::AddDots4()
{
	std::vector<std::pair<int, int>> srcDots;
	std::vector<std::pair<int, int>> dstDots;

	{  // src
		for (int j = 25; j < 39; ++j)
		{
			for (int i = 40; i < 80; ++i)
			{
				srcDots.push_back({ j, i });
			}
		}
		for (int j = 25; j < 53; ++j)
		{
			for (int i = 80; i < 110; ++i)
			{
				srcDots.push_back({ j, i });
			}
		}
		for (int j = 25; j < 45; ++j)
		{
			for (int i = 110; i < 140; ++i)
			{
				srcDots.push_back({ j, i });
			}
		}
		for (int j = 25; j < 43; ++j)
		{
			for (int i = 140; i < 170; ++i)
			{
				srcDots.push_back({ j, i });
			}
		}
		for (int j = 25; j < 35; ++j)
		{
			for (int i = 170; i < 210; ++i)
			{
				srcDots.push_back({ j, i });
			}
		}
		for (int j = 25; j < 47; ++j)
		{
			for (int i = 210; i < 250; ++i)
			{
				srcDots.push_back({ j, i });
			}
		}
	}

	{  // dst
		for (int j = 644; j > 630; --j)
		{
			for (int i = 530 - 1; i >= 490; --i)
			{
				dstDots.push_back({ j, i });
			}
		}
		for (int j = 658; j > 630; --j)
		{
			for (int i = 490 - 1; i >= 460; --i)
			{
				dstDots.push_back({ j, i });
			}
		}
		for (int j = 650; j > 630; --j)
		{
			for (int i = 460 - 1; i >= 430; --i)
			{
				dstDots.push_back({ j, i });
			}
		}
		for (int j = 648; j > 630; --j)
		{
			for (int i = 430 - 1; i >= 400; --i)
			{
				dstDots.push_back({ j, i });
			}
		}
		for (int j = 640; j > 630; --j)
		{
			for (int i = 400 - 1; i >= 360; --i)
			{
				dstDots.push_back({ j, i });
			}
		}
		for (int j = 652; j > 630; --j)
		{
			for (int i = 360 - 1; i >= 320; --i)
			{
				dstDots.push_back({ j, i });
			}
		}
	}

	size_t size = srcDots.size();
	assert(size == dstDots.size());

	for (size_t j = 0; j < size; ++j)
	{
		auto& dst = dstDots[j];
		auto& src = srcDots[j];

		field[dst.first][dst.second] = { src.first - dst.first, src.second - dst.second };
		transformField[dst.first][dst.second] = { -field[src.first][src.second].first, -field[src.first][src.second].second };
	}
}
