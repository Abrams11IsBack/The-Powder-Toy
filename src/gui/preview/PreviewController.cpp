#include "PreviewController.h"
#include "Controller.h"
#include "PreviewModel.h"
#include "PreviewModelException.h"
#include "PreviewView.h"
#include "client/Client.h"
#include "client/SaveInfo.h"
#include "client/GameSave.h"
#include "common/platform/Platform.h"
#include "graphics/Graphics.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/login/LoginController.h"
#include "gui/login/LoginView.h"
#include "Config.h"

PreviewController::PreviewController(int saveID, int saveDate, bool instant, std::function<void ()> onDone_, std::unique_ptr<VideoBuffer> thumbnail):
	saveId(saveID),
	loginWindow(NULL),
	HasExited(false)
{
	previewModel = new PreviewModel();
	previewView = new PreviewView(std::move(thumbnail));
	previewModel->AddObserver(previewView);
	previewView->AttachController(this);
	previewModel->SetDoOpen(instant);

	previewModel->UpdateSave(saveID, saveDate);

	if(Client::Ref().GetAuthUser().UserID)
	{
		previewModel->SetCommentBoxEnabled(true);
	}

	Client::Ref().AddListener(this);

	onDone = onDone_;
}

void PreviewController::Update()
{
	previewModel->Update();
	if (loginWindow && loginWindow->HasExited == true)
	{
		delete loginWindow;
		loginWindow = NULL;
	}
	if (previewModel->GetDoOpen() && previewModel->GetSaveInfo() && previewModel->GetSaveInfo()->GetGameSave())
	{
		Exit();
	}
}

bool PreviewController::SubmitComment(String comment)
{
	if(comment.length() < 4)
	{
		new ErrorMessage("Error", "Comment is too short");
		return false;
	}
	else
	{
		RequestStatus status = Client::Ref().AddComment(saveId, comment);
		if(status != RequestOkay)
		{
			new ErrorMessage("Error submitting comment", Client::Ref().GetLastError());
			return false;
		}
		else
		{
			previewModel->CommentAdded();
			previewModel->UpdateComments(1);
		}
	}
	return true;
}

void PreviewController::ShowLogin()
{
	loginWindow = new LoginController();
	loginWindow->GetView()->MakeActiveWindow();
}

void PreviewController::NotifyAuthUserChanged(Client * sender)
{
	previewModel->SetCommentBoxEnabled(sender->GetAuthUser().UserID);
}

const SaveInfo *PreviewController::GetSaveInfo() const
{
	return previewModel->GetSaveInfo();
}

std::unique_ptr<SaveInfo> PreviewController::TakeSaveInfo()
{
	return previewModel->TakeSaveInfo();
}

bool PreviewController::GetDoOpen()
{
	return previewModel->GetDoOpen();
}

void PreviewController::DoOpen()
{
	previewModel->SetDoOpen(true);
}

void PreviewController::Report(String message)
{
	if(Client::Ref().ReportSave(saveId, message) == RequestOkay)
	{
		Exit();
		new InformationMessage("Information", "Report submitted", false);
	}
	else
		new ErrorMessage("Error", "Unable to file report: " + Client::Ref().GetLastError());
}

void PreviewController::FavouriteSave()
{
	if(previewModel->GetSaveInfo() && Client::Ref().GetAuthUser().UserID)
	{
		try
		{
			if(previewModel->GetSaveInfo()->Favourite)
				previewModel->SetFavourite(false);
			else
				previewModel->SetFavourite(true);
		}
		catch (PreviewModelException & e)
		{
			new ErrorMessage("Error", ByteString(e.what()).FromUtf8());
		}
	}
}

void PreviewController::OpenInBrowser()
{
	ByteString uri = ByteString::Build(SCHEME, SERVER, "/Browse/View.html?ID=", saveId);
	Platform::OpenURI(uri);
}

bool PreviewController::NextCommentPage()
{
	if(previewModel->GetCommentsPageNum() < previewModel->GetCommentsPageCount() && previewModel->GetCommentsLoaded() && !previewModel->GetDoOpen())
	{
		previewModel->UpdateComments(previewModel->GetCommentsPageNum()+1);
		return true;
	}
	return false;
}

bool PreviewController::PrevCommentPage()
{
	if(previewModel->GetCommentsPageNum() > 1 && previewModel->GetCommentsLoaded() && !previewModel->GetDoOpen())
	{
		previewModel->UpdateComments(previewModel->GetCommentsPageNum()-1);
		return true;
	}
	return false;
}

void PreviewController::Exit()
{
	previewView->CloseActiveWindow();
	HasExited = true;
	if (onDone)
		onDone();
}

PreviewController::~PreviewController()
{
	previewView->CloseActiveWindow();
	Client::Ref().RemoveListener(this);
	delete previewModel;
	delete previewView;
}
