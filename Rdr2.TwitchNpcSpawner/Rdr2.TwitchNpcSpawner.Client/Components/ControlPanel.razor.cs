namespace Rdr2.TwitchNpcSpawner.Client.Components;

using Rdr2.TwitchNpcSpawner.Client.Data;
using Rdr2.TwitchNpcSpawner.Client.Pages;
using Rdr2.TwitchNpcSpawner.Client.Shared;
using Microsoft.AspNetCore.Components;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Net.Http.Json;
using System.Threading.Tasks;

public partial class ControlPanel : ComponentBase
{
    [Inject]
    public required NavigationManager NavMgr { get; init; }

    [Parameter, EditorRequired]
    public GameState? GameState { get; init; }

    [CascadingParameter]
    public required Home Parent { get; init; }

    bool _isInitialized = false;
    string? _jwt;
    TwitchUser? _user;
    List<CustomReward> _rewards = new();

    bool _isEnablingReward = false;
    bool _isFailedToEnableReward = false;
    bool _isDisablingReward = false;
    bool _isFailedToDisableReward = false;

    CustomReward? _editing;

    protected override async Task OnAfterRenderAsync(bool firstRender)
    {
        if (!firstRender)
            return;

        try
        {
            using var http = new HttpClient();

            {
                using var response = await http.GetAsync("http://127.0.0.1:41078/getjwt");
                if (!response.IsSuccessStatusCode)
                    return;

                _jwt = await response.Content.ReadAsStringAsync();
                if (_jwt is null)
                    return;
            }

            http.DefaultRequestHeaders.Add("Authorization", _jwt);

            {
                using var response = await http.GetAsync(NavMgr.BaseUri + "Auth/Validate");
                if (!response.IsSuccessStatusCode)
                {
                    _jwt = null;
                    return;
                }
            }

            {
                using var response = await http.GetAsync(NavMgr.BaseUri + "User");
                if (response.IsSuccessStatusCode)
                    _user = await response.Content.ReadFromJsonAsync<TwitchUser>();
            }

            await Sync();
        }
        finally
        {
            _isInitialized = true;
            StateHasChanged();
        }
    }

    private async Task Sync()
    {
        using var http = new HttpClient();
        http.DefaultRequestHeaders.Add("Authorization", _jwt);
        {
            using var response = await http.GetAsync(NavMgr.BaseUri + "Reward/Sync");
            if (!response.IsSuccessStatusCode)
            {
                _jwt = null;
                return;
            }
        }

        {
            using var response = await http.GetAsync(NavMgr.BaseUri + "Reward/List");
            if (response.IsSuccessStatusCode)
                _rewards = await response.Content.ReadFromJsonAsync<List<CustomReward>>() ?? new();
        }
    }

    private string? GetBestImage(Dictionary<string, string>? imgs)
    {
        if (imgs is null || imgs.Count == 0)
            return "/img/custom-reward.png";

        var pairs = imgs.OrderByDescending(x => int.TryParse(new string(x.Key.Where(xx => char.IsDigit(xx)).ToArray()), out var n) ? n : -1);
        return pairs.FirstOrDefault().Value;
    }

    private async Task EnableReward(CustomReward reward)
    {
        if (_isEnablingReward)
            return;
        _isEnablingReward = true;
        _isFailedToEnableReward = false;
        _isFailedToDisableReward = false;

        try
        {
            using var http = new HttpClient();
            http.DefaultRequestHeaders.Add("Authorization", _jwt);
            using var form = new FormUrlEncodedContent(new Dictionary<string, string>
            {
                ["id"] = reward.Id.ToString()
            });
            using var response = await http.PostAsync(NavMgr.BaseUri + "Reward/Enable", form);
            if (!response.IsSuccessStatusCode)
            {
                _isFailedToEnableReward = true;
                await Sync();
                return;
            }

            var updatedReward = await response.Content.ReadFromJsonAsync<CustomReward>();
            if (updatedReward is null)
            {
                await Sync();
                return;
            }

            var index = _rewards.FindIndex(x => x.Id == updatedReward.Id);
            if (index < 0)
                _rewards.Add(updatedReward);
            else
                _rewards[index] = updatedReward;
        }
        finally
        {
            _isEnablingReward = false;
        }
    }

    private async Task DisableReward(CustomReward reward)
    {
        if (_isDisablingReward)
            return;
        _isDisablingReward = true;
        _isFailedToDisableReward = false;
        _isFailedToDisableReward = false;

        try
        {
            using var http = new HttpClient();
            http.DefaultRequestHeaders.Add("Authorization", _jwt);
            using var form = new FormUrlEncodedContent(new Dictionary<string, string>
            {
                ["id"] = reward.Id.ToString()
            });
            using var response = await http.PostAsync(NavMgr.BaseUri + "Reward/Disable", form);
            if (!response.IsSuccessStatusCode)
            {
                _isFailedToDisableReward = true;
                await Sync();
                return;
            }

            var updatedReward = await response.Content.ReadFromJsonAsync<CustomReward>();
            if (updatedReward is null)
            {
                await Sync();
                return;
            }

            var index = _rewards.FindIndex(x => x.Id == updatedReward.Id);
            if (index < 0)
                _rewards.Add(updatedReward);
            else
                _rewards[index] = updatedReward;
        }
        finally
        {
            _isDisablingReward = false;
        }
    }

    public async Task Save(CustomReward reward, CustomRewardReq req)
    {
        try
        {
            using var http = new HttpClient();
            http.DefaultRequestHeaders.Add("Authorization", _jwt);
            using var response = await http.PostAsJsonAsync(NavMgr.BaseUri + "Reward/Update/" + reward.Id, req);
            if (!response.IsSuccessStatusCode)
            {
                await Sync();
                return;
            }

            var updatedReward = await response.Content.ReadFromJsonAsync<CustomReward>();
            if (updatedReward is null)
            {
                await Sync();
                return;
            }

            var index = _rewards.FindIndex(x => x.Id == updatedReward.Id);
            if (index < 0)
                _rewards.Add(updatedReward);
            else
                _rewards[index] = updatedReward;
        }
        finally
        {
            _editing = null;
            StateHasChanged();
        }
    }

    public void CancelSave()
    {
        _editing = null;
        StateHasChanged();
    }

    public async Task KickPed(long handle)
    {
        using var http = new HttpClient();
        using var response = await http.GetAsync("http://127.0.0.1:41078/kick/" + handle);
        await Task.Delay(500);
        Parent.Sync();
    }

    public async Task KickAll()
    {
        using var http = new HttpClient();
        using var response = await http.GetAsync("http://127.0.0.1:41078/kickall");
        await Task.Delay(500);
        Parent.Sync();
    }

    public async Task Disconnect()
    {
        using var http = new HttpClient();
        using var response = await http.GetAsync("http://127.0.0.1:41078/disconnect");
        await Task.Delay(500);
        Parent.Sync();
    }

    public async Task TryReconnect()
    {
        using var http = new HttpClient();
        using var response = await http.GetAsync("http://127.0.0.1:41078/tryreconnect");
        await Task.Delay(500);
        Parent.Sync();
    }

    public async Task Logout()
    {
        using var http = new HttpClient();
        using var response = await http.GetAsync("http://127.0.0.1:41078/logout");
        await Task.Delay(500);
        Parent.Sync();
    }
}
