namespace Rdr2.TwitchNpcSpawner.Client.Components;

using Rdr2.TwitchNpcSpawner.Client.Shared;
using Microsoft.AspNetCore.Components;

public partial class EditCustomReward : ComponentBase
{
    [Parameter, EditorRequired]
    public required CustomReward Reward { get; set; }

    [CascadingParameter]
    public required ControlPanel ControlPanel { get; set; }

    private CustomRewardReq? _req;
    private int _rewardId = -1;
    private bool _isSaving = false;

    protected override void OnParametersSet()
    {
        if (_rewardId == Reward.Id)
            return;
        _rewardId = Reward.Id;
        _isSaving = false;

        if (Reward.TwitchModel is null)
        {
            _req = null;
            return;
        }

        _req = new()
        {
            title = Reward.TwitchModel.title ?? "",
            cost = Reward.TwitchModel.cost,
            is_user_input_required = Reward.TwitchModel.is_user_input_required,
            prompt = Reward.TwitchModel.prompt,
            is_enabled = Reward.TwitchModel.is_enabled,
            background_color = Reward.TwitchModel.background_color,
            is_max_per_stream_enabled = Reward.TwitchModel.max_per_stream_setting?.is_enabled ?? false,
            max_per_stream = Reward.TwitchModel.max_per_stream_setting?.max_per_stream ?? 1,
            is_max_per_user_per_stream_enabled = Reward.TwitchModel.max_per_user_per_stream_setting?.is_enabled ?? false,
            max_per_user_per_stream = Reward.TwitchModel.max_per_user_per_stream_setting?.max_per_user_per_stream ?? 1,
            is_global_cooldown_enabled = Reward.TwitchModel.global_cooldown_setting?.is_enabled ?? false,
            global_cooldown_seconds = Reward.TwitchModel.global_cooldown_setting?.global_cooldown_seconds ?? 90,
            should_redemptions_skip_request_queue = Reward.TwitchModel.should_redemptions_skip_request_queue,
            extra = Reward.Extra
        };
    }

    private async Task Save()
    {
        if (_req is null)
            return;

        if (_isSaving)
            return;
        _isSaving = true;

        try
        {
            await ControlPanel.Save(Reward, _req);
        }
        finally
        {
            _isSaving = false;
        }
    }

    private void Cancel()
    {
        ControlPanel.CancelSave();
    }
}
