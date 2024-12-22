#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./drivers/display/display_board.h"

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

#define MAX_NUM_USERS 6

enum state
{
	LOGIN,
	VALID_ID,
	INVALID_ID,
	VALID_PIN,
	INVALID_PIN,
	ADMIN,
	CARD_DETECTED
};

enum stateADMIN
{
	SELECTING_MODE,
	NEWUSER,
	CHANGEPIN,
	DELETEUSER,
	BRIGHTNESS,
	EXIT
};

enum stateSelecting
{
	SELECTING,
	BEGIN_SELECTING
};


enum stateNeewUser
{
	CREATE_USER,
	REGISTER_PIN,
	LINK_CARD
};

enum registerPINflags
{
	REG_FINISHED, 
	REG_STARTED, 
	REG_ERROR
};

typedef struct User
{
	uint8_t ID[8];
	uint8_t ID_symbolIndex[8];
	uint64_t primaryAccount;
	uint8_t PIN[5];
	bool isPIN4;
	// bool isAdmin;
	uint8_t index;
    uint8_t floor;
	bool inBuilding;
} UserProfile_t;

UserProfile_t usersLUT[MAX_NUM_USERS] = {
	{
		.ID = {D_1, D_2, D_3, D_4, D_5, D_6, D_7, D_8},
		.ID_symbolIndex = {1, 2, 3, 4, 5, 6, 7, 8},
		.PIN = {D_1, D_3, D_3, D_4, D_7},
		.primaryAccount = 6391300354579455,
		.isPIN4 = false,
		.floor = 1,
		.inBuilding = false
	},
	{
		.ID = {D_2, D_2, D_2, D_2, D_2, D_2, D_2, D_2},
		.ID_symbolIndex = {2,2,2,2,2,2,2,2},
		.PIN = {D_1, D_3, D_3, D_4, D_7},
		.primaryAccount = 6031670912054111975,
		.isPIN4 = false,
		.floor = 1,
		.inBuilding = false
	},
	{
		.ID = {D_3, D_3, D_3, D_3, D_3, D_3, D_3, D_3},
		.ID_symbolIndex = {3,3,3,3,3,3,3,3},
		.PIN = {D_1, D_3, D_3, D_4, D_7},
		.primaryAccount = 5258550770854513,
		.isPIN4 = false,
		.floor = 2,
		.inBuilding = false
	},
	{
		.ID = {D_4, D_4, D_4, D_4, D_4, D_4, D_4, D_4},
		.ID_symbolIndex = {4,4,4,4,4,4,4,4},
		.PIN = {D_1, D_3, D_3, D_4, D_7},
		.primaryAccount = 5273417513549969,
		.isPIN4 = false,
		.floor = 2,
		.inBuilding = false
	},
	{
		.ID = {D_5, D_5, D_5, D_5, D_5, D_5, D_5, D_5},
		.ID_symbolIndex = {5,5,5,5,5,5,5,5},
		.PIN = {D_1, D_3, D_3, D_4, D_7},
		.primaryAccount = 6031670912048084317,
		.isPIN4 = false,
		.floor = 3,
		.inBuilding = false
	},
	{
		.ID = {D_6, D_6, D_6, D_6, D_6, D_6, D_6, D_6},
		.ID_symbolIndex = {6,6,6,6,6,6,6,6},
		.PIN = {D_1, D_3, D_3, D_4, D_7},
		.primaryAccount = 4066633064906002,
		.isPIN4 = false,
		.floor = 3,
		.inBuilding = false
	}
};


/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
//  ******************************************************************************/
/**
 * @brief Verifies that the ID entered by the user is valid
 */
bool checkID();

/**
 * @brief Verifies that the PIN entered by the user is valid
 */
bool checkPIN();

/**
 * @brief Verifies is two Arrays of the same length are equal
 * 
 * @param Arr2Check 
 * @param Arr2Compare
 * @param length
 */
bool areArraysEqual(uint8_t *Arr2Check, uint8_t *Arr2Compare, int length);

/**
 * @brief Turns on RED LED to indicate ACCESS DENIED (Invalid ID or PIN)
 */
void invalid_LED(void);

/**
 * @brief Turns on GREEN LED to indicate ACCESS GRANTED (Valid ID and PIN)
 */
void valid_LED(void);

/** 
 * @brief Turns on BLUE LED to indicate ADMIN mode
 */
void admin_LED(void);

/**
 * @brief Turns on BLUE LED & GREEN LED to indicate that the ID is valid
 */
void validID_LED(void);

/**
 * @brief Turns on ALL LEDS to indicate LOGIN mode.
 */
void login_LED(void);

/**
 * @brief Resets currentUser struct
 */
void resetCurrentUser(void);

/**
 * @brief Callback function for Timer interrupt. Increments PeriodTime counter.
 */
void getTime(void);


/*
 * @brief Initializes the users array with the hardcoded user.
 */
void initializeUsers();



/**
 * @brief Prints "ErrOr" in the displays.
 */
void printError(void);
