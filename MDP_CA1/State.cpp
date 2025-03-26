#include "State.hpp"
#include "StateID.hpp"
#include "StateStack.hpp"

State::Context::Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, MusicPlayer& music, SoundPlayer& sounds, KeyBinding& key1, KeyBinding& key2, MultiplayerManager& multiplayer_manager)
    : window(&window)
      , textures(&textures)
      , fonts(&fonts)
      , music(&music)
      , sounds(&sounds)
      , keys1(&key1)
      , keys2(&key2)
	  , multiplayer_manager(&multiplayer_manager)
      
{
}

State::State(StateStack& stack, Context context) : m_stack(&stack), m_context(context)
{
}

State::~State()
{
}

void State::RequestStackPush(StateID state_id)
{
    m_stack->PushState(state_id);
}

void State::RequestStackPop()
{
    m_stack->PopState();
}

void State::RequestStackClear()
{
    m_stack->ClearStack();
}

State::Context State::GetContext() const
{
    return m_context;
}

void State::OnActivate()
{
}
void State::OnDestroy()
{
}

void State::OnStackPopped()
{
    //do nothing
}
