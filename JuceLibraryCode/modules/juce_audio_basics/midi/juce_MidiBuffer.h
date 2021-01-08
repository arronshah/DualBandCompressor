/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#ifndef JUCE_MIDIBUFFER_H_INCLUDED
#define JUCE_MIDIBUFFER_H_INCLUDED


//==============================================================================
/**
    Holds a sequence of time-stamped midi events.

    Analogous to the AudioSampleBuffer, this holds a set of midi events with
    integer time-stamps. The buffer is kept sorted in order of the time-stamps.

    If you're working with a sequence of midi events that may need to be manipulated
    or read/written to a midi file, then MidiMessageSequence is probably a more
    appropriate container. MidiBuffer is designed for lower-level streams of raw
    midi data.

    @see MidiMessage
*/
class JUCE_API  MidiBuffer
{
public:
    //==============================================================================
    /** Creates an empty MidiBuffer. */
    MidiBuffer() noexcept;

    /** Creates a MidiBuffer containing a single midi message. */
    explicit MidiBuffer (const MidiMessage& message) noexcept;

    /** Creates a copy of another MidiBuffer. */
    MidiBuffer (const MidiBuffer& other) noexcept;

    /** Makes a copy of another MidiBuffer. */
    MidiBuffer& operator= (const MidiBuffer& other) noexcept;

    /** Destructor */
    ~MidiBuffer();

    //==============================================================================
    /** Removes all events from the buffer. */
    void clear() noexcept;

    /** Removes all events between two times from the buffer.

        All events for which (start <= event position < start + numSamples) will
        be removed.
    */
    void clear (int start, int numSamples);

    /** Returns true if the buffer is empty.

        To actually retrieve the events, use a MidiBuffer::Iterator object
    */
    bool isEmpty() const noexcept;

    /** Counts the number of events in the buffer.

        This is actually quite a slow operation, as it has to iterate through all
        the events, so you might prefer to call isEmpty() if that's all you need
        to know.
    */
    int getNumEvents() const noexcept;

    /** Adds an event to the buffer.

        The sample number will be used to determine the position of the event in
        the buffer, which is always kept sorted. The MidiMessage's timestamp is
        ignored.

        If an event is added whose sample position is the same as one or more events
        already in the buffer, the new event will be placed after the existing ones.

        To retrieve events, use a MidiBuffer::Iterator object
    */
    void addEvent (const MidiMessage& midiMessage, int sampleNumber);

    /** Adds an event to the buffer from raw midi data.

        The sample number will be used to determine the position of the event in
        the buffer, which is always kept sorted.

        If an event is added whose sample position is the same as one or more events
        already in the buffer, the new event will be placed after the existing ones.

        The event data will be inspected to calculate the number of bytes in length that
        the midi event really takes up, so maxBytesOfMidiData may be longer than the data
        that actually gets stored. E.g. if you pass in a note-on and a length of 4 bytes,
        it'll actually only store 3 bytes. If the midi data is invalid, it might not
        add an event at all.

        To retrieve events, use a MidiBuffer::Iterator object
    */
    void addEvent (const void* rawMidiData,
                   int maxBytesOfMidiData,
                   int sampleNumber);

    /** Adds some events from another buffer to this one.

        @param otherBuffer          the buffer containing the events you want to add
        @param startSample          the lowest sample number in the source buffer for which
                                    events should be added. Any source events whose timestamp is
                                    less than this will be ignored
        @param numSamples           the valid range of samples from the source buffer for which
                                    events should be added - i.e. events in the source buffer whose
                                    timestamp is greater than or equal to (startSample + numSamples)
                                    will be ignored. If this value is less than 0, all events after
                                    startSample will be taken.
        @param sampleDeltaToAdd     a value which will be added to the source timestamps of the events
                                    that are added to this buffer
    */
    void addEvents (const MidiBuffer& otherBuffer,
                    int startSample,
                    int numSamples,
                    int sampleDeltaToAdd);

    /** Returns the sample number of the first event in the buffer.

        If the buffer's empty, this will just return 0.
    */
    int getFirstEventTime() const noexcept;

    /** Returns the sample number of the last event in the buffer.

        If the buffer's empty, this will just return 0.
    */
    int getLastEventTime() const noexcept;

    //==============================================================================
    /** Exchanges the contents of this buffer with another one.

        This is a quick operation, because no memory allocating or copying is done, it
        just swaps the internal state of the two buffers.
    */
    void swapWith (MidiBuffer& other) noexcept;

    /** Preallocates some memory for the buffer to use.
        This helps to avoid needing to reallocate space when the buffer has messages
        added to it.
    */
    void ensureSize (size_t minimumNumBytes);

    //==============================================================================
    /**
        Used to iterate through the events in a MidiBuffer.

        Note that altering the buffer while an iterator is using it isn't a
        safe operation.

        @see MidiBuffer
    */
    class JUCE_API  Iterator
    {
    public:
        //==============================================================================
        /** Creates an Iterator for this MidiBuffer. */
        Iterator (const MidiBuffer& buffer) noexcept;

        /** Destructor. */
        ~Iterator() noexcept;

        //==============================================================================
        /** Repositions the iterator so that the next event retrieved will be the first
            one whose sample position is at greater than or equal to the given position.
        */
        void setNextSamplePosition (int samplePosition) noexcept;

        /** Retrieves a copy of the next event from the buffer.

            @param result   on return, this will be the message (the MidiMessage's timestamp
                            is not set)
            @param samplePosition   on return, this will be the position of the event
            @returns        true if an event was found, or false if the iterator has reached
                            the end of the buffer
        */
        bool getNextEvent (MidiMessage& result,
                           int& samplePosition) noexcept;

        /** Retrieves the next event from the buffer.

            @param midiData     on return, this pointer will be set to a block of data containing
                                the midi message. Note that to make it fast, this is a pointer
                                directly into the MidiBuffer's internal data, so is only valid
                                temporarily until the MidiBuffer is altered.
            @param numBytesOfMidiData   on return, this is the number of bytes of data used by the
                                        midi message
            @param samplePosition   on return, this will be the position of the event
            @returns        true if an event was found, or false if the iterator has reached
                            the end of the buffer
        */
        bool getNextEvent (const uint8* &midiData,
                           int& numBytesOfMidiData,
                           int& samplePosition) noexcept;

    private:
        //==============================================================================
        const MidiBuffer& buffer;
        const uint8* data;

        JUCE_DECLARE_NON_COPYABLE (Iterator)
    };

private:
    //==============================================================================
    friend class MidiBuffer::Iterator;
    MemoryBlock data;
    int bytesUsed;

    uint8* getData() const noexcept;
    uint8* findEventAfter (uint8*, int samplePosition) const noexcept;

    JUCE_LEAK_DETECTOR (MidiBuffer)
};


#endif   // JUCE_MIDIBUFFER_H_INCLUDED
