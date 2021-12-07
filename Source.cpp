#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>

#include "SDL_Wrapper.h"

using namespace std;

/* ENUM, ARRAY AND STRUCT DEFINITIONS */

enum GameState { PlayerTurn, DealerTurn, GameOver };

enum Suit { Clubs, Diamonds, Hearts, Spades };
string SuitStrings[] = { "Clubs", "Diamonds", "Hearts", "Spades" };

enum   Value            {
	Ace, Two, Three, Four, Five, Six, Seven, Eight, Nine,
	Ten, Jack, Queen, King
};
string ValueStrings[] = { "Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine",
"Ten", "Jack", "Queen", "King" };

struct Card {
	Value  value;
	Suit   suit;
};

struct Deck {
	Card cards[52];
	int next_card;
};

/* Hand represents a pile of cards */

struct Hand {
	int num_cards;
	Card cards[52];	// holds UP TO 52 cards
};

/* GLOBALS*/

Image CardImages[52];

/* FUNCTIONS */

void InitCardImages() {
	Image Cards = LoadImage("Cards.png");
	int slot = 0;
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < 4; j++) {
			int x = (2000 * i) / 13;
			int y = (1117 * j) / 5;
			CardImages[slot] = CropImage(Cards, x, y, 154, 223);
			slot++;
		}
	}
}

int GetCardCode(Card c) {
	return c.value * 4 + c.suit;
}

string CardToString(Card c) {
	return ValueStrings[c.value] + " of " + SuitStrings[c.suit];
}

int RandInRange(int low, int high) {
	int range = high - low + 1;
	return (rand() % range) + low;
}

void FillDeck(Deck& deck) {
	int slot = 0;
	for (int value = 0; value < 13; value++) {
		for (int suit = 0; suit < 4; suit++) {
			deck.cards[slot].value = (Value)value;
			deck.cards[slot].suit = (Suit)suit;
			slot++;
		}
	}
	deck.next_card = 0;
}

void PrintDeck(Deck deck) {
	for (int i = 0; i < 52; i++)
		cout << CardToString(deck.cards[i]) << endl;
}

void ShuffleDeck(Deck& deck) {
	for (int i = 0; i < 52; i++) {
		int j = RandInRange(i, 51);
		Card temp = deck.cards[i];
		deck.cards[i] = deck.cards[j];
		deck.cards[j] = temp;
	}
	deck.next_card = 0;
}

Card DealCard(Deck& deck) {
	Card out = deck.cards[deck.next_card];
	deck.next_card++;
	return out;
}


void InitializeHand(Hand& cs) {
	// what would be good starting values for each member of the struct?
	cs.num_cards = 0;
}

void AddCardToHand(Hand& cs, Card cardToAdd) {
	// what needs to happen when we add a card to a card stack?
	cs.cards[cs.num_cards] = cardToAdd;
	cs.num_cards++;

}

void DrawHand(const Hand& cs, int x, int y) {
	// should draw a stack of cards starting at x,y and going diagonally down...
	for (int i = 0; i < cs.num_cards; i++) {
		Image img = CardImages[GetCardCode(cs.cards[i])];
		DrawImage(img, x, y);
		x += 20;
		y += 20;
	}
}

int GetPoints(Hand hand) {
	int points = 0;
	int ace_count = 0;
	for (int i = 0; i < hand.num_cards; i++) {
		Card c = hand.cards[i];
		switch (c.value) {
		case Ace:
			points += 11;
			ace_count++;
			break;
		case Jack:
		case Queen:
		case King:
			points += 10;
			break;
		default:
			points += 1 + c.value;
			break;
		}
	}
	while (points > 21 && ace_count > 0) {
		points -= 10;
		ace_count--;
	}
	return points;
}

// cacophony
int main(int argc, char ** argv) {
	InitSystem(1280, 720);
	srand(time(NULL));

	InitCardImages();
	
	Sound intro = LoadSound("NewGame.wav");
	Sound deal_card = LoadSound("DealCard.wav");
	Sound next_turn = LoadSound("NextTurn.wav");
	Sound you_lost = LoadSound("YouLost.wav");
	Sound you_win = LoadSound("YouWin.wav");
	Music popStyle = LoadMusic("PopStyle.mp3");
	PlaySound(intro);
	PlayMusic(popStyle,2);
	Deck deck;
	FillDeck(deck);
	ShuffleDeck(deck);

	Hand playerHand, dealerHand;
	InitializeHand(dealerHand);
	InitializeHand(playerHand);

	GameState state = PlayerTurn;
	int delay = 0;
	int wins = 0, losses = 0,ties=0;
	FillRect(0, 0, 1280, 720, DarkBlue);
	while (IsKeyDown('q') == false) {
		if (state == PlayerTurn) {
			// check if space was pressed; if so, deal a card from the deck 
			// and add it to the stack
			if (WasKeyPressed(SpaceKey)) {
				Card c = DealCard(deck);
				PlaySound(deal_card);
				AddCardToHand(playerHand, c);
			}
			if (WasKeyPressed('d')) {
				state = DealerTurn;
				delay = 180; // --STEVE
				PlaySound(next_turn);
			}
			if (GetPoints(playerHand)>21){
				WriteString("Bust", 220, 30);
				PlaySound(next_turn);
			}
		}
		if (state == DealerTurn) {
			delay--;
			if (delay <= 0) {
				if (GetPoints(dealerHand) < 17) {
					Card c = DealCard(deck);
					PlaySound(deal_card);
					AddCardToHand(dealerHand, c);
					delay = 50;
				}
				else {
					state = GameOver;
					if (GetPoints(playerHand) > 21)
						losses++;
					else if (GetPoints(playerHand) > 21) {
						losses++;
						PlaySound(you_lost);

					}
					else if (GetPoints(dealerHand) > 21) {
						wins++;
						PlaySound(you_win);

					}
					// else if (the dealer TIED OR BEAT the player)...
					else if (GetPoints(dealerHand) == GetPoints(playerHand)){
						ties++;
					}
					//             give the player a loss
					else if (GetPoints(dealerHand) >GetPoints(playerHand) && GetPoints(dealerHand) != GetPoints(playerHand)){
						losses++;
						PlaySound(you_lost);

					}
					// else player gets a win.
					else if (GetPoints(dealerHand) < GetPoints(playerHand)){
						wins++;
						PlaySound(you_win);

					}
				}
			}
		}
		if (state == GameOver) {
			// if they press space, go back to PlayerTurn, but:
			if (WasKeyPressed(SpaceKey)) {
				state = PlayerTurn;
				//   Shuffle the Deck;
				ShuffleDeck(deck);
				//   Reinitialize both the player and dealer's hands;
				InitializeHand(playerHand);
				InitializeHand(dealerHand);
				//   Set the game state to Player's turn;
				state = PlayerTurn;
				//   Set delay to 0.
				delay = 0;
			}
		}

		// Draw
		FillRect(0, 0, 1280, 720, DarkBlue);
		DrawHand(playerHand, 50, 50);
		WriteInt(GetPoints(playerHand), 210, 30);
		DrawHand(dealerHand, 550, 50);
		WriteInt(GetPoints(dealerHand), 500, 30);
		WriteString("Wins: ", 0, 0);
		WriteInt(wins, 120, 0);
		WriteString("Ties: ", 240, 0);
		WriteInt(ties, 360, 0);
		WriteString("Losses: ",480, 0);
		WriteInt(losses, 600, 0);
		if (GetPoints(playerHand)>21){
			WriteString("Bust", 250, 30);
		}
		if (GetPoints(dealerHand)>21){
			WriteString("Bust", 700, 30);
		}
		// maybe draw the deck too?? (face down of course)
		// write a message: Space to hit, enter to stay
		Refresh();
		Sleep(10);
	}

	CloseSystem();
	return 0;
}