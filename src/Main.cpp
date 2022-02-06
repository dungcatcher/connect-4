#include "Platform/Platform.hpp"
#include <stdint.h>

uint64_t redPieces = 0;
uint64_t yellowPieces = 0;

// const uint64_t bitColumns[] = { 0x40810204081, 0x81020408102, 0x102040810204, 0x204081020408, 0x408102040810, 0x810204081020, 0x1020408102040 };

//  Board representation
//  35 36 37 38 39 40 41
//  28 29 30 31 32 33 34
//  21 22 23 24 25 26 27
//  14 15 16 17 18 19 20
//  7  8  9  10 11 12 13
//  0  1  2   3  4  5  6

bool detectWin(uint64_t board);
bool detectWin(uint64_t board)
{
	// Check south east diagonal
	uint64_t tempBoard = board & (board >> 6);
	if (tempBoard & (tempBoard >> 2 * 6))
		return true;
	// Check vertical
	tempBoard = board & (board >> 7);
	if (tempBoard & (tempBoard >> 2 * 7))
		return true;
	// Check north east diagonal
	tempBoard = board & (board >> 8);
	if (tempBoard & (tempBoard >> 2 * 8))
		return true;
	// Check horizontal
	tempBoard = board & (board >> 1);
	if (tempBoard & (tempBoard >> 2 * 1))
		return true;

	return false;
}

void updateBoard(sf::RenderWindow* window, sf::Sprite* boardSprite, bool turn, float ssf);
void updateBoard(sf::RenderWindow* window, sf::Sprite* boardSprite, bool turn, float ssf)
{
	window->clear();
	window->draw(*boardSprite);

	sf::CircleShape redPiece(40 * ssf);
	redPiece.setFillColor(sf::Color::Red);
	redPiece.setOrigin(sf::Vector2f(40 * ssf, 40 * ssf));
	sf::CircleShape yellowPiece(40 * ssf);
	yellowPiece.setFillColor(sf::Color::Yellow);
	yellowPiece.setOrigin(sf::Vector2f(40 * ssf, 40 * ssf));

	if (detectWin((turn) ? yellowPieces : redPieces)) // opposite turn as the other person just played
	{
		std::cout << "win\n";
	}

	int testBit = 0;
	uint64_t checkRedBits = redPieces;
	uint64_t checkYellowBits = yellowPieces;

	while (testBit < 42)
	{
		if (checkRedBits & 0x01)
		{
			const int xPos = testBit % 7;
			const int yPos = 5 - testBit / 7;
			redPiece.setPosition(sf::Vector2f(xPos * (window->getSize().x / 7) + window->getSize().x / 14, yPos * (window->getSize().y / 6) + window->getSize().y / 12));
			window->draw(redPiece);
		}
		if (checkYellowBits & 0x01)
		{
			const int xPos = testBit % 7;
			const int yPos = 5 - testBit / 7;
			yellowPiece.setPosition(sf::Vector2f(xPos * (window->getSize().x / 7) + window->getSize().x / 14, yPos * (window->getSize().y / 6) + window->getSize().y / 12));
			window->draw(yellowPiece);
		}

		testBit++;
		checkRedBits >>= 1;
		checkYellowBits >>= 1;
	}
	window->display();
}

int generateMove(int column, uint64_t redBoard, uint64_t yellowBoard);
int generateMove(int column, uint64_t redBoard, uint64_t yellowBoard)
{
	// Returns the y value of the column clicked on
	uint64_t vacantPositions = ~(redBoard | yellowBoard);
	vacantPositions >>= column;
	int row = -1; // If row is -1 there are no legal spaces

	for (unsigned int i = 0; i < 6; i++)
	{
		if (vacantPositions & 0x01)
		{
			row = i;
			break;
		}
		vacantPositions >>= 7;
	}

	return row;
}

uint64_t makeMove(int row, int col, uint64_t board);
uint64_t makeMove(int row, int col, uint64_t board)
{
	board |= ((uint64_t)1 << (row * 7 + col));
	return board;
}

long int perft(int currentPly, int maxPly, bool turn, uint64_t redBoard, uint64_t yellowBoard);
long int perft(int currentPly, int maxPly, bool turn, uint64_t redBoard, uint64_t yellowBoard)
{
	long int nodes = 0;

	const bool continueSearch = currentPly < maxPly;

	for (int col = 0; col < 7; col++)
	{
		int row = generateMove(col, redBoard, yellowBoard);
		if (row != -1)
		{
			if (continueSearch)
			{
				nodes += perft(currentPly + 1, maxPly, !turn, (turn) ? makeMove(row, col, redBoard) : redBoard, (turn) ? yellowBoard : makeMove(row, col, yellowBoard));
			}
			else
			{
				nodes++;
			}
		}
	}
	return nodes;
}

int main()
{
	util::Platform platform;

#if defined(_DEBUG)
	std::cout << "Hello World!" << std::endl;
#endif

	sf::RenderWindow window;
	// in Windows at least, this must be called before creating the window
	float screenScalingFactor = platform.getScreenScalingFactor(window.getSystemHandle());
	// Use the screenScalingFactor
	window.create(sf::VideoMode(700.0f * screenScalingFactor, 600.0f * screenScalingFactor), "Connect 4");
	platform.setIcon(window.getSystemHandle());

	sf::Texture boardTexture;
	boardTexture.loadFromFile("./content/board.png");
	sf::Sprite boardSprite;
	boardSprite.setTexture(boardTexture);
	boardSprite.setScale(sf::Vector2f(screenScalingFactor, screenScalingFactor));

	bool turn = 1; // 1 = red, 0 = yellow
	bool mouseDown = false;

	std::cout << perft(1, 7, 1, redPieces, yellowPieces) << "\n";

	window.setFramerateLimit(60);

	sf::Event event;

	while (window.isOpen())
	{
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
			{
				if (!mouseDown)
				{
					int column = sf::Mouse::getPosition(window).x / (window.getSize().x / 7);
					int row = generateMove(column, redPieces, yellowPieces);
					if (row != -1)
					{
						unsigned int bitPosition = row * 7 + column;
						std::cout << bitPosition << "\n";
						if (turn)
						{
							redPieces |= ((uint64_t)1 << bitPosition); // Edit red pieces bit
							turn = 0;
						}
						else
						{
							yellowPieces |= ((uint64_t)1 << bitPosition); // Edit yellow pieces bit
							turn = 1;
						}
					}
				}
				mouseDown = true;
			}
			else
				mouseDown = false;
		}

		updateBoard(&window, &boardSprite, turn, screenScalingFactor);
	}

	return 0;
}
