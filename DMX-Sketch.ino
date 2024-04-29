#define UNIVERSE_COUNT 1
#define MAX_ADDR_VAL 255

#include <stdio.h>
#include <assert.h>

#include "lib/DMXSerial-1.5.3/src/DMXSerial.cpp"
#include "lib/DMXSerial-1.5.3/src/DMXSerial.h"
#include "lib/DMXSerial-1.5.3/src/DMXSerial_avr.h"

struct DMXChannel
{
  // Starting Address of the Channel; Convention is to start at 1 and increment by 20
  int startingAddress;
  uint8_t intensity; // Intensity of the Channel (0-255)
};

struct DMXUniverse
{
  // Max number of channels in a universe
  // Derived by using convention of starting at 1 and incrementing by 20
  const static int maxChannels = 25;
  DMXChannel channels[DMXUniverse::maxChannels]; // Channels in the Universe
};

// Used Globally
DMXUniverse *universes[UNIVERSE_COUNT];

// Used for Serial Instruction Communication
const byte maxInstrLen = 100;
char receivedInstr[maxInstrLen];
bool newData = false;
String universeStr = "";
String intensityStr = "";

// Used for Instruction Parsing
int state = 0;
String tempChannelVal = "";
int rangeStart = 0;
int rangeEnd = 0;
int uni_val;
int int_val;
int selectedChannels[DMXUniverse::maxChannels];

// ========================================================
// Initalization Methods
// ========================================================

/**
 * Initializes the given number of universes with the max number of channels
 *
 * @param num The number of universes to initialize
 */
void initUniverses(int num)
{
  for (int i = 0; i < num; i++)
  {
    universes[i] = new DMXUniverse();

    for (int j = 0; j < DMXUniverse::maxChannels; j++)
    {
      universes[i]->channels[j] = DMXChannel{
          j * 20 + 1, // Starting Address
          0           // Intensity
      };

      // Print out the new channel with all relevant info
      getChannelInfo(&universes[i]->channels[j], j);
    };
  }
}

// ========================================================
// DMXUniverse Methods
// ========================================================

/**
 * Returns the channel at the given index (starting from 1)
 *
 * @param universe The universe to get the channel from
 * @param num The index of the channel to get
 */
DMXChannel *getChannelByNumber(DMXUniverse *universe, int num)
{
  assert(num > 0 && num < DMXUniverse::maxChannels);

  return &universe->channels[num - 1];
}

/**
 * Returns a range of channels from the given start to end index
 *
 * @param universe The universe to get the channels from
 * @param start The starting index of the range
 * @param end The ending index of the range
 */
DMXChannel **getChannelRange(DMXUniverse *universe, int start, int end)
{
  assert(start < end);
  assert(start > 0 && start < DMXUniverse::maxChannels);
  assert(end > 0 && end < DMXUniverse::maxChannels);

  int numToGet = end - start + 1;

  DMXChannel **channelRange = malloc(numToGet * sizeof(DMXChannel *));
  for (int i = start; i <= end; i++)
  {
    channelRange[i - start] = getChannelByNumber(universe, i);
  }

  return channelRange;
}

/**
 * Returns all channels in the given universe
 *
 * @param universe The universe to get the channels from
 */
DMXChannel **getAllChannels(DMXUniverse *universe)
{
  return getChannelRange(universe, 1, DMXUniverse::maxChannels);
}

/**
 * Prints the state of the given universe for debugging
 *
 * @param universe The universe to print the state of
 * @param index The index of the universe (used for printing)
 */
void getUniverseInfo(DMXUniverse *universe, int index)
{
  Serial.print("Universe ");
  Serial.print(index + 1);
  Serial.println();
  Serial.print("=================================");
  Serial.println();

  for (int i = 0; i < DMXUniverse::maxChannels; i++)
  {
    getChannelInfo(&universe->channels[i], i);
  }
}

// ========================================================
// DMXChannel Methods
// ========================================================

/**
 * Sets the intensity of the given channel
 *
 * @param channel The channel to set the intensity of
 * @param newVal The new intensity value
 */
void setChannelValue(DMXChannel **channel, int newVal)
{
  DMXChannel *chan = malloc(sizeof(DMXChannel));
  chan->startingAddress = (*channel)->startingAddress;
  chan->intensity = newVal;
  *channel = chan;

  sendAddressData(chan);
}

// void setChannelRangeValue(DMXChannel **channels, int newVal) {
//   int channelCount = sizeof(channels) / sizeof(DMXChannel **);

//   for (int i = 0; i < channelCount; i++) {
//     setChannelValue(channels[i], newVal);
//   }
// }

/**
 * Prints the state of the given channel for debugging
 *
 * @param channel The channel to print the state of
 * @param index The index of the channel (used for printing)
 */
void getChannelInfo(DMXChannel *channel, int index)
{
  Serial.print("Channel ");
  Serial.print(index);
  Serial.print(" (");
  Serial.print(channel->startingAddress);
  Serial.print(") : Intensity = ");
  Serial.print(channel->intensity);
  Serial.println();
}

// ========================================================
// Arduino Methods
// ========================================================

void setup()
{
  DMXSerial.init(DMXController); // Initialize DMX

  Serial.begin(9600); // Computer
  delay(2000);        // Wait for Serial to start

  // Initialize the Universes
  initUniverses(UNIVERSE_COUNT);
}

void loop()
{
  readInput();
  processData();
}

// Taken from https://forum.arduino.cc/t/serial-input-basics-updated/382007

/**
 * Reads the input from the serial monitor
 */
void readInput()
{
  char endMarker = '\n';
  while (!Serial.available() || newData)
    ;
  Serial.readBytesUntil(endMarker, receivedInstr, maxInstrLen);
  newData = true;
}

/**
 * Processes the input from the serial monitor
 */
void processData()
{
  if (!newData)
    return;

  // Everything
  printInput();
  matchInput();
  updateUniverse();

  // Reset State
  memset(receivedInstr, 0, maxInstrLen);
  universeStr = "";
  intensityStr = "";
  newData = false;
}

/**
 * Prints the input from the serial monitor
 */
void printInput()
{
  Serial.println(receivedInstr);
}

/**
 * Matches the input from the serial monitor to the correct universe, intensity, and channels
 */
void matchInput()
{
  // Split the input into Universe, Intensity, and Channels
  char *addr = strtok(receivedInstr, " ");
  strtok(NULL, " ");
  char *intensity = strtok(NULL, " ");
  char *universe = strtok(addr, "/");
  char *channels = strtok(NULL, "");

  // Convert Universe & Intensity to ints
  uni_val = String(universe).toInt();
  int_val = String(intensity).toInt();

  // Verify they are within bounds
  if (!uni_val)
    return;
  if (int_val < 0 || int_val > 100)
    return;
  if (uni_val < 1 || uni_val > UNIVERSE_COUNT)
    return;

  // Reset Selected Channels
  memset(selectedChannels, 0, DMXUniverse::maxChannels);
  int scIndex = 0;
  int i = 0;

  // Parse the Channels
  do
  {
    char currByte = channels[i];
    switch (state)
    {
    // Start
    case 0:
      if (isDigit(currByte))
      {
        tempChannelVal = tempChannelVal + currByte;
        state = 1;
      }
      else
        state = -1;
      break;
    case 1:
      // Int Received
      // If at terminator, enact state 2
      if (currByte == '\0')
      {
        // Commit the last channel if valid
        int toCommit = tempChannelVal.toInt();
        if (toCommit < DMXUniverse::maxChannels && toCommit > 0)
          selectedChannels[scIndex] = toCommit;
        tempChannelVal = "";
        state = -1;
        break;
      }

      if (isDigit(currByte))
      {
        // If digit, add to tempChannelVal
        // Could be a multi-digit number
        tempChannelVal = tempChannelVal + currByte;
        break;
      }

      if (currByte == ',')
      {
        // If comma, commit the current channel if valid
        int toCommit = tempChannelVal.toInt();
        if (toCommit < DMXUniverse::maxChannels && toCommit > 0)
          selectedChannels[scIndex] = toCommit;
        tempChannelVal = "";
        scIndex++;
        state = 0;
        break;
      }

      if (currByte == '-')
      {
        rangeStart = tempChannelVal.toInt();
        if (rangeStart > DMXUniverse::maxChannels || rangeStart <= 0)
        {
          state = -1;
          break;
        }

        // Clear tempChannelVal for reuse, move to end range state
        tempChannelVal = "";
        state = 3;
        break;
      }

      state = -1;
      break;
    case 3:
      // If we terminate, commit the range
      if (currByte == '\0' || currByte == ',')
      {
        int rangeEnd = tempChannelVal.toInt();
        if (tempChannelVal != "" && rangeEnd <= DMXUniverse::maxChannels && rangeEnd > 0)
        {
          for (int j = rangeStart; j <= rangeEnd; j++)
          {
            selectedChannels[scIndex] = j;
            scIndex++;
          }
        }
        tempChannelVal = "";
        // If we receive a comma, go back to state 0 to start a new channel
        // Otherwise, terminate
        state = (currByte == ',') ? 0 : -1;
        break;
      }

      // If we receive a digit, add to tempChannelVal
      // Could be a multi-digit number
      if (isDigit(currByte))
      {
        tempChannelVal = tempChannelVal + currByte;
      }
      break;
    default:
      // If we reach an invalid state, terminate
      scIndex = DMXUniverse::maxChannels;
    }
    i++;
  } while (scIndex < DMXUniverse::maxChannels && channels[i - 1] != '\0');

  // Reset State
  i = 0;
  rangeStart = 0;
  rangeEnd = 0;
  tempChannelVal = "";
  state = 0;

  // Print the Parsed Data
  Serial.print("Universe ");
  Serial.println(uni_val);
  Serial.print("Channels ");

  for (int x : selectedChannels)
  {
    if (x == 0)
      break;
    Serial.print(x);
    Serial.print(",");
  }
  scIndex = 0;
  Serial.println("");

  // Serial.println(channels);
  Serial.print("Intensity ");
  Serial.println(int_val);
}

/**
 * Updates the universe with the new intensity values
 */
void updateUniverse()
{
  for (int c : selectedChannels)
  {
    if (c == 0)
      break;

    DMXChannel *selChan = getChannelByNumber(universes[uni_val - 1], c);
    setChannelValue(&selChan, int_val);
  }
}

/**
 * Sends the address data to the DMX controller
 *
 * @param channel The channel to send the data for
 */
void sendAddressData(DMXChannel *channel)
{
  int startingAddr = channel->startingAddress;
  int intensity = channel->intensity;

  // Proof of Concept for what the data should look like
  uint8_t data[10] = {
      255, // Red
      255, // Lime
      255, // Amber
      255, // Green
      255, // Cyan
      255, // Blue
      255, // Indigo
      intensity,
      255, // Strobe
      255, // Fan Control
  };

  // Print the Starting Address
  // Serial.print("Starting Address: ");
  // Serial.println(startingAddr);

  // Send the data to the lights
  for (int i = 0; i < 10; i++)
  {
    DMXSerial.write(startingAddr + i, data[i]);
  }
}