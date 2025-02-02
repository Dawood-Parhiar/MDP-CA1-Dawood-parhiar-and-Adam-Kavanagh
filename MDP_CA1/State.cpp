#include "State.hpp"
#include "StateID.hpp"
#include "StateStack.hpp"

State::Context::Context(sf::RenderWindow& window, TextureHolder& textures, FontHolder& fonts, MusicPlayer& music, SoundPlayer& sounds, Player& p1, Player& p2)
    : window(&window)
      , textures(&textures)
      , fonts(&fonts)
      , music(&music)
      , sounds(&sounds)
      , player(&p1)
      , player2(&p2)
      
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