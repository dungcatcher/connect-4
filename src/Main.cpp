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
	// std::cout << vacantPositicurrentNode->checkedCol = currentNode->checkedCol + 1;ons << "\n";
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
				const auto newBoard = (turn) ? makeMove(row, col, redBoard) : makeMove(row, col, yellowBoard);
				if (detectWin(newBoard))
					return 0;
				nodes += perft(currentPly + 1, maxPly, !turn, (turn) ? newBoard : redBoard, (turn) ? yellowBoard : newBoard);
			}
			else
			{
				nodes++;
			}
		}
	}
	return nodes;
}

struct perftThreadInfo
{
	int maxPly;
	bool turn;
	uint64_t redBoard;
	uint64_t yellowBoard;
	sf::Mutex* mutex;
	long int* nodes;
	uint startingCol;
};

// void updateGlobaPerft(int* maxPly, bool* turn, uint64_t* redBoard, uint64_t* yellowBoard, sf::Mutex* mutex, long int* nodes);
// void updateGlobaPerft(int* maxPly, bool* turn, uint64_t* redBoard, uint64_t* yellowBoard, sf::Mutex* mutex, long int* nodes)
void updateGlobaPerft(perftThreadInfo info);
void updateGlobaPerft(perftThreadInfo info)
{
	const auto nodesToAdd = perft(2, info.maxPly, info.turn, info.redBoard, info.yellowBoard);
	std::cout << "Nodes to add from starting col " << info.startingCol << ": " << nodesToAdd << "\n";
	info.mutex->lock();
	*info.nodes += nodesToAdd;
	info.mutex->unlock();
}

long int multiThreadedPerft(int maxPly, bool turn, uint64_t redBoard, uint64_t yellowBoard);
long int multiThreadedPerft(int maxPly, bool turn, uint64_t redBoard, uint64_t yellowBoard)
{
	long int nodes = 0;
	sf::Mutex nodesMutex;
	perftThreadInfo threadInfo;
	threadInfo.maxPly = maxPly;
	threadInfo.turn = turn;
	threadInfo.mutex = &nodesMutex;
	threadInfo.nodes = &nodes;

	const bool continueSearch = 1 < maxPly;

	std::thread threads[7];
	for (int col = 0; col < 7; col++)
	{
		int row = generateMove(col, redBoard, yellowBoard);
		if (row != -1)
		{
			if (continueSearch)
			{
				const auto newBoard = (turn) ? makeMove(row, col, redBoard) : makeMove(row, col, yellowBoard);
				if (detectWin(newBoard))
					nodes++;
				else
				{
					if (turn)
					{
						threadInfo.redBoard = newBoard;
						threadInfo.yellowBoard = yellowBoard;
					}
					else
					{
						threadInfo.redBoard = newBoard;
						threadInfo.yellowBoard = yellowBoard;
					}
					threadInfo.startingCol = col;
					threads[col] = std::thread(&updateGlobaPerft, threadInfo);
				}
			}
			else
				nodes++;
		}
	}

	for (size_t i = 0; i < 7; i++)
		if (threads[i].joinable())
			threads[i].join();

	std::cout << "Multi Threaded Perft Results with Ply " << maxPly << ": " << nodes << "\n";

	return nodes;
}

// 0 is board1 and is Red
// 1 is board2 and is Yellow

void solveConnect4();
void solveConnect4()
{
	std::thread threads[49];
	for (int col = 0; col < 7; col++)
	{
		uint64_t board1 = makeMove(0, col, 0);
		for (int col2 = 0; col2 < 7; col2++)
		{
			int row = generateMove(col2, board1, 0);
			uint64_t board2 = makeMove(row, col2, 0);
			UNUSED(board2);
		}
	}
}

struct node
{
	node* parent;
	std::vector<node*> children = {};
	uint64_t board1;
	uint64_t board2;
	int col;
	int checkedCol;
	bool isLoss;
	bool turn; // just played
	int depth;
};

void deleteNodeChildrenRecurrsive(node* deleteNode);
void deleteNodeChildrenRecurrsive(node* deleteNode)
{
	for (auto child : deleteNode->children)
	{
		deleteNodeChildrenRecurrsive(child);
		delete child;
	}
}

void displayNodeLayer(node* currentNode);
void displayNodeLayer(node* currentNode)
{
	for (int i = 0; i < currentNode->depth; i++)
	{
		std::cout << '-';
	}
	std::cout << " isLoss: " << currentNode->isLoss << " Boards: " << currentNode->board1 << " | " << currentNode->board2;
}

void displayNodeTree(node* startingNode);
void displayNodeTree(node* startingNode)
{
	std::cout << "Done\n";
	displayNodeLayer(startingNode);
}

void solveFromThread();
void solveFromThread()
{
	node startingNode;
	startingNode.board1 = 0;
	startingNode.board2 = 0;
	startingNode.depth = 0;
	startingNode.checkedCol = 0;
	startingNode.turn = 1; // yellow just played therefore red now plays
	startingNode.parent = nullptr;
	startingNode.isLoss = false;
	startingNode.col = 100;

	node* currentNode = &startingNode;
	while (true)
	{
		// std::cout << currentNode->depth << " | " << currentNode->col << "\n";
		if (currentNode->depth == 0 && currentNode != &startingNode)
		{
			displayNodeTree(currentNode);
			return;
		}
		else if (currentNode->isLoss)
		{
			// std::cout << "Node is loss\n";
			deleteNodeChildrenRecurrsive(currentNode);
			currentNode->children.clear();
			if (!currentNode->turn)
				// yellow just played
				currentNode->parent->isLoss = true;
			currentNode = currentNode->parent;
		}
		else
		{
			if (currentNode->checkedCol < 7)
			{
				int row = generateMove(currentNode->checkedCol, currentNode->board1, currentNode->board2);
				if (row != -1)
				{
					node* newNode;
					newNode = new node;

					// turn ? red just played : yellow
					newNode->board1 = (currentNode->turn) ? currentNode->board1 : makeMove(row, currentNode->checkedCol, currentNode->board1);
					newNode->board2 = (!currentNode->turn) ? currentNode->board2 : makeMove(row, currentNode->checkedCol, currentNode->board2);
					newNode->parent = currentNode;
					newNode->depth = currentNode->depth + 1;
					newNode->col = currentNode->checkedCol;
					newNode->turn = !currentNode->turn;
					newNode->isLoss = false;
					newNode->checkedCol = 0;
					newNode->children = {};

					currentNode->children.push_back(newNode);
					currentNode->checkedCol = currentNode->checkedCol + 1;

					currentNode = newNode;

					if (detectWin((currentNode->turn) ? currentNode->board1 : currentNode->board2))
					{
						std::cout << "Win" << ((currentNode->turn) ? currentNode->board1 : currentNode->board2) << "\n";
						if (currentNode->turn)
						{
							// go to parent node
							currentNode = currentNode->parent;
						}
						else
						{
							// prune tree
							currentNode->parent->isLoss = true;
							currentNode = currentNode->parent;
						}
					}
				}
				else
				{
					// std::cout << "Illegal\n";
					currentNode->checkedCol = currentNode->checkedCol + 1;
				}
			}
			else
			{
				// checked all sub nodes

				// now check if all sub nodes are a loss
				bool isLoss = true;
				for (auto child : currentNode->children)
					if (!child->isLoss)
						isLoss = false;
				if (isLoss)
					currentNode->isLoss = true;
				else
					currentNode = currentNode->parent;
			}
		}
	}
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

	solveFromThread();
	// multiThreadedPerft(12, 1, redPieces, yellowPieces);
	// perft(1, 12, true, 0, 0);

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
