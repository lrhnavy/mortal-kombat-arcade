#include "Game.h"
#include "AnimatorHolder.h"
#include "MusicPlayer.h"

bool Game::start = false;
bool Game::EndOfGame = false;

Game::Game() {
	timeAnimator = new TickTimerAnimator(NULL);
	timeAnimation = new TickTimerAnimation(111);
	round = 1;
	camera = { STAGE_WIDTH / 2 - SCREEN_WIDTH / 2, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
};

Game::~Game() {
	CleanUp();
};

bool Game::initialize(SDL_Surface* gScreenSurface) {
	AnimationFilm* tmp = AnimationFilmHolder::Get()->GetFilm("stage");
	background = tmp->GetBitmap();
	AnimationFilm* temp = AnimationFilmHolder::Get()->GetFilm("bckg");
	movingBckg = temp->GetBitmap();
	//Init fonts
	Timerfont = TTF_OpenFont("media/font.ttf", 70);
	if (Timerfont == NULL)
	{
		cout << "Failed to load lazy font! SDL_ttf Error: %s\n" << TTF_GetError();
		return false;
	}

	Namefont = TTF_OpenFont("media/font.ttf", 45);
	if (Namefont == NULL)
	{
		cout << "Failed to load lazy font! SDL_ttf Error: %s\n" << TTF_GetError();
		return false;
	}
	//call the initialization of players

	subzero = new Fighter("subzero", { 580,500 });
	scorpion = new Fighter("scorpion", { 1280,500 });
	if (!subzero->initialize("config/subzero_controller.json")) return false;
	if (!scorpion->initialize("config/scorpion_controller.json")) return false;

	return true;
};


void Game::DrawGame(SDL_Surface& gScreenSurface) {

	collisionNhits(*subzero, *scorpion);
	collisionNhits(*scorpion, *subzero);

	MatchEnd(gScreenSurface);
	cameraAdjustment();

	SDL_Rect fullscreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	SDL_BlitScaled(movingBckg, 0, &gScreenSurface, &fullscreen);
	SDL_BlitSurface(background, &camera, &gScreenSurface, &fullscreen);
	//For debugging purposes the timer is big 
	printTimer(gameTimer.ReverseTimer(60), { SCREEN_WIDTH / 2 - 35, 5 }, &gScreenSurface, { 198, 0, 10, 255 });

	if (!start&&timeAnimator->GetState() == ANIMATOR_RUNNING) {
		scorpion->SetState("READY");
		subzero->SetState("READY");
		scorpion->ResetHealth();
		subzero->ResetHealth();
		subzero->ResetPosition({ 580,500 });
		scorpion->ResetPosition({ 1280,500 });

		printMessage("Round " + to_string(round), { SCREEN_WIDTH / 2 - 180,SCREEN_HEIGHT / 2 - 200 }, &gScreenSurface, { 255, 255, 0, 255 }, 150);
	}

	//The camera might need moving or interaction with the playerres 
	if (rand() % 2 + 1 == 2) {
		scorpion->Draw(gScreenSurface, subzero->GetPosition(), camera);
		subzero->Draw(gScreenSurface, scorpion->GetPosition(), camera);
	}
	else {//illusion of being at the same z-order they never must collapse
		subzero->Draw(gScreenSurface, scorpion->GetPosition(), camera);
		scorpion->Draw(gScreenSurface, subzero->GetPosition(), camera);
	}

	//x,y, height/width changed the orientation in function
	RenderHpBarLeft(subzero->getHealth(), gScreenSurface);
	RenderHpBarRight(scorpion->getHealth(), gScreenSurface);
};

void Game::CleanUp() {
	//Fix this 
	//Clean fonts surfaces and all
	//call it at winow

};

void Game::HandleInput(SDL_Event& event) {
	if (!Game::start) {
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_SPACE && !EndOfGame) {//&& timeAnimator->GetState() != ANIMATOR_RUNNING) {//Here check for tick animator
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);
					Game::start = true;
					scorpion->SetState("READY");
					subzero->SetState("READY");
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("fight"), 0);
					gameTimer.start();
				}, 1000);//A bit more time here but for debug purposes leave it fast
			}
		}
	}
	else {
		subzero->Handler();
		scorpion->Handler();
	}
};


void Game::printTimer(const std::string& msg, Point position, SDL_Surface *gScreenSurface, SDL_Color color) {
	SDL_Rect dest = { position.x,position.y,0,0 };
	SDL_Surface *stext = TTF_RenderText_Blended(Timerfont, msg.c_str(), color);

	if (stext) {
		SDL_BlitSurface(stext, NULL, gScreenSurface, &dest);
		SDL_FreeSurface(stext);
	}
	else {
		throw
			std::string("Couldn't allocate text surface in printMessageAt");
	}
};

void Game::RenderHpBarRight(float healt, SDL_Surface& gScreenSurface) {
	Rect bar = { 679, 65, 45, 580 };
	healt = healt > 1.f ? 1.f : healt < 0.f ? 0.f : healt;

	SDL_Surface* tmp = SDL_CreateRGBSurface(0, bar.h, bar.w, 32, 0, 0, 0, 0);
	SDL_FillRect(tmp, NULL, SDL_MapRGB(tmp->format, 198, 0, 10));
	SDL_BlitSurface(tmp, NULL, &gScreenSurface, &bar);
	SDL_FreeSurface(tmp);

	int pw = (int)((float)bar.w * healt);

	tmp = SDL_CreateRGBSurface(0, pw, bar.h, 32, 0, 0, 0, 0);
	SDL_FillRect(tmp, NULL, SDL_MapRGB(tmp->format, 0, 128, 0));
	Rect tempRect = { bar.x + (bar.w - pw),bar.y,bar.w,bar.h };
	SDL_BlitSurface(tmp, NULL, &gScreenSurface, &tempRect);
	SDL_FreeSurface(tmp);

	//NAME 
	SDL_Rect dest = { 1055,65,0,0 };
	SDL_Surface *stext = TTF_RenderText_Blended(Namefont, "SCORPION", { 255, 255, 0, 255 });
	if (stext) {
		SDL_BlitSurface(stext, NULL, &gScreenSurface, &dest);
		SDL_FreeSurface(stext);
	}
	else {
		throw
			std::string("Couldn't allocate text surface in printMessageAt");
	}
	tmp = AnimationFilmHolder::Get()->GetFilm("win")->GetBitmap();

	Rect displayWin = { 1210,110,50,50 };
	if (scorpion->GetWin() == 1) {
		SDL_BlitScaled(tmp, NULL, &gScreenSurface, &displayWin);
	}
	else if (scorpion->GetWin() == 2) {
		SDL_BlitScaled(tmp, NULL, &gScreenSurface, &displayWin);
		displayWin.x = displayWin.x - 50;
		SDL_BlitScaled(tmp, NULL, &gScreenSurface, &displayWin);
	}
};

void Game::RenderHpBarLeft(float healt, SDL_Surface& gScreenSurface) {
	Rect bar = { 24, 65, 45, 580 };
	healt = healt > 1.f ? 1.f : healt < 0.f ? 0.f : healt;

	SDL_Surface* tmp = SDL_CreateRGBSurface(0, bar.h, bar.w, 32, 0, 0, 0, 0);
	SDL_FillRect(tmp, NULL, SDL_MapRGB(tmp->format, 198, 0, 10));
	SDL_BlitSurface(tmp, NULL, &gScreenSurface, &bar);
	SDL_FreeSurface(tmp);

	int pw = (int)((float)bar.w * healt);

	tmp = SDL_CreateRGBSurface(0, pw, bar.h, 32, 0, 0, 0, 0);
	SDL_FillRect(tmp, NULL, SDL_MapRGB(tmp->format, 0, 128, 0));
	SDL_BlitSurface(tmp, NULL, &gScreenSurface, &bar);
	SDL_FreeSurface(tmp);

	//Name
	SDL_Rect dest = { 50,65,0,0 };
	SDL_Surface *stext = TTF_RenderText_Blended(Namefont, "SUBZERO", { 255, 255, 0, 255 });
	if (stext) {
		SDL_BlitSurface(stext, NULL, &gScreenSurface, &dest);
		SDL_FreeSurface(stext);
	}
	else {
		throw
			std::string("Couldn't allocate text surface in printMessageAt");
	}
	tmp = AnimationFilmHolder::Get()->GetFilm("win")->GetBitmap();

	Rect displayWin = { 555,110,50,50 };
	if (subzero->GetWin() == 1) {
		SDL_BlitScaled(tmp, NULL, &gScreenSurface, &displayWin);
	}
	else if (subzero->GetWin() == 2) {
		SDL_BlitScaled(tmp, NULL, &gScreenSurface, &displayWin);
		displayWin.x = displayWin.x - 50;
		SDL_BlitScaled(tmp, NULL, &gScreenSurface, &displayWin);
	}
};

void Game::DelayAction(const std::function<void()>& f, delay_t d) {
	if (timeAnimator&&timeAnimator->GetState() != ANIMATOR_RUNNING) {
		timeAnimation->setOnTick([] {
			//Nothing to do here
		}).SetDelay(d).SetReps(1);
		timeAnimator = new TickTimerAnimator(timeAnimation);
		timeAnimator->SetOnFinish(f);
		timeAnimator->Start(SDL_GetTicks());
		AnimatorHolder::MarkAsRunning(timeAnimator);
	}
};


void Game::printMessage(const string& msg, Point position, SDL_Surface *gScreenSurface, SDL_Color color, int fontsize) {

	SDL_Rect dest = { position.x,position.y,0,0 };
	tmpFont = TTF_OpenFont("media/font.ttf", fontsize);
	if (Namefont == NULL)
	{
		throw
			std::string("Failed to load lazy font! SDL_ttf Error");
	}
	SDL_Surface *stext = TTF_RenderText_Blended(tmpFont, msg.c_str(), color);

	if (stext) {
		SDL_BlitSurface(stext, NULL, gScreenSurface, &dest);
		SDL_FreeSurface(stext);
		TTF_CloseFont(tmpFont);
	}
	else {
		throw
			std::string("Couldn't allocate text surface in printMessageAt");
		TTF_CloseFont(tmpFont);
	}
};


void Game::cameraAdjustment() {
	if (subzero->GetPosition().x < camera.x) camera.x = camera.x - 10;
	if (scorpion->GetPosition().x + 194 > camera.x + SCREEN_WIDTH) camera.x = camera.x + 10;
	if (camera.x < 0) camera.x = 0;
	if (camera.x > STAGE_WIDTH - SCREEN_WIDTH) camera.x = STAGE_WIDTH - SCREEN_WIDTH;
};

void Game::matchWin(Fighter& fighter, SDL_Surface& gScreenSurface) {
	MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect(fighter.GetName() + ".wins"), 0);
	fighter.WinAnimation();
	if (fighter.GetWin() >= 2)
		EndOfGame = true;
	else round++;
};

int Game::GetRound(void) const {
	return round;
};

void Game::MatchEnd(SDL_Surface& gScreenSurface) {
	if (!gameTimer.isStarted() && start) {//timer stopped
		Game::start = false;
		if (subzero->getHealth() > scorpion->getHealth()) {
			subzero->SetWin();
			matchWin(*subzero, gScreenSurface);
		}
		else if (subzero->getHealth() < scorpion->getHealth()) {
			scorpion->SetWin();
			matchWin(*scorpion, gScreenSurface);
		}
		else {
			if (rand() % 2) {
				scorpion->SetWin();
				matchWin(*scorpion, gScreenSurface);
			}
			else {
				subzero->SetWin();
				matchWin(*subzero, gScreenSurface);
			}
		}
	}
	else if (subzero->getHealth() == 0 && start) {
		Game::start = false;
		gameTimer.stop();
		scorpion->SetWin();
		matchWin(*scorpion, gScreenSurface);
	}
	else if (scorpion->getHealth() == 0 && start) {
		Game::start = false;
		gameTimer.stop();
		subzero->SetWin();
		matchWin(*subzero, gScreenSurface);
	}
};

void Game::collisionNhits(Fighter& hitter, Fighter& hitted) {
	if (hitter.proximityDetector(hitted.GetSprite())) {
		/*
		 *NORMAL ATTACKS
		 */
		if (hitter.GetAction()._Equal("punch") || hitter.GetAction()._Equal("kick")) {
			if (hitted.GetState()._Equal("BLOCK")) {
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);//Hit blocked
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("block"), 0);
					hitter.fightstasts.blocked++;
				}, hitter.GetAction()._Equal("punch") ? 350 : 850);

			}
			else if (hitted.GetState()._Equal("BLOCKDWN") || hitted.GetState()._Equal("UP") || hitted.GetState()._Equal("DOWN")) {
				//Nothing happens
			}
			else {

				//Here reduce health maybe depending on hit 
				if (hitter.GetAction()._Equal("punch")) {
					hitted.removeHealth(PUNCH_DMG);
				}
				else {
					hitted.removeHealth(KICK_DMG);
				}

				//sound && inflictions animations
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("singlehit"), 0);
					hitted.InflictionAnimation("singlehit", 50, hitter.GetAction()._Equal("punch") ? "punch" : "kick");
				}, hitter.GetAction()._Equal("punch") ? 350 : 850);
				//blood and tears			
			}
		}/*
		 *UP ATTACKS
		 */
		else if (hitter.GetAction()._Equal("uppunch") || hitter.GetAction()._Equal("upkick")) {
			if (hitted.GetState()._Equal("BLOCK") || hitted.GetState()._Equal("BLOCKDWN")) {
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);//Hit blocked
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("block"), 0);
					hitter.fightstasts.blocked++;
				}, 750);
			}
			else {
				//Here reduce health maybe depending on hit 
				if (hitter.GetAction()._Equal("uppunch")) {
					hitted.removeHealth(UPPERCUT_DMG);
				}
				else {
					hitted.removeHealth(KICK_DMG);
				}

				//sound && inflictions animations
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);
					hitted.InflictionAnimation("singlehit", 50, hitter.GetAction()._Equal("punch") ? "punch" : "kick");
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("singlehit"), 0);
				}, 750);
				//blood and tears	
			}
		}
		/*
		 *DOWN ATTACKS
		 */
		else if (hitter.GetAction()._Equal("downpunch") || hitter.GetAction()._Equal("downkick")) {
			if (hitted.GetState()._Equal("BLOCKDWN")) {
				//Block hit 
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);//Hit blocked
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("block"), 0);
					hitter.fightstasts.blocked++;
				}, hitter.GetAction()._Equal("downpunch") ? 650 : 250);

			}
			else if (hitted.GetState()._Equal("UP")) {
				//Nothing happens
			}
			else {
				//Here reduce health maybe depending on hit 
				if (hitter.GetAction()._Equal("downpunch")) {
					hitted.removeHealth(PUNCH_DMG);
				}
				else {
					hitted.removeHealth(KICK_DMG);
				}

				//sound && inflictions animations
				DelayAction([&]() {
					AnimatorHolder::Remove(timeAnimator);
					MusicPlayer::Get()->PlayEffect(MusicPlayer::Get()->RetrieveEffect("singlehit"), 0);
					rand() % 3 + 1 == 2 ? hitted.InflictionAnimation("uppercuthit", 100, hitter.GetAction()._Equal("downpunch") ? "punch" : "kick") : hitted.InflictionAnimation("singlehit", 50, hitter.GetAction()._Equal("downpunch") ? "punch" : "kick");
				}, hitter.GetAction()._Equal("downpunch") ? 650 : 250);
				//blood and tears
			}
		}//do a check for special combos also
	}
};

Fighter* Game::GetWinner(void) {
	return subzero->GetWin() >= 2 ? subzero : scorpion;
};

Fighter* Game::GetLoser(void) {
	return subzero->GetWin() >= 2 ? scorpion : subzero;
};

