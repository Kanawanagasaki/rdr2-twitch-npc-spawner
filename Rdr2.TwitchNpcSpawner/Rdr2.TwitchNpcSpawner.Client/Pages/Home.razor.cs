namespace Rdr2.TwitchNpcSpawner.Client.Pages;

using Rdr2.TwitchNpcSpawner.Client.Data;
using Microsoft.AspNetCore.Components;
using Microsoft.JSInterop;
using System.Net.Http.Json;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Timers;

public partial class Home
{
    [Inject]
    public required NavigationManager NavMgr { get; init; }

    [Parameter, SupplyParameterFromQuery(Name = "code")]
    public string? Code { get; init; }

    Timer? _timer;
    HttpClient _http = new HttpClient();
    bool _isLoaded = false;
    GameState? _state;
    EGameStatus _status = EGameStatus.Initial;
    bool _isLogginigWithTwitch = false;
    bool _isFailedToLoginWithTwitch = false;

    protected override async Task OnAfterRenderAsync(bool firstRender)
    {
        if (firstRender)
        {
            if (Code is not null)
            {
                try
                {
                    using var http = new HttpClient();
                    using var response = await http.GetAsync(NavMgr.BaseUri + "Auth/Code?code=" + Code);
                    response.EnsureSuccessStatusCode();

                    var json = await response.Content.ReadFromJsonAsync<OAuthCodeResponse>();
                    if (json is not null)
                    {
                        await http.PostAsJsonAsync("http://127.0.0.1:41078/setjwt", json);
                        _status = EGameStatus.Unknown;
                    }
                }
                catch (Exception e)
                {
                    Console.Error.WriteLine(e.Message);
                }

                NavMgr.NavigateTo(NavMgr.BaseUri, true);
            }

            _isLoaded = true;
            _timer = new Timer();
            _timer.Enabled = true;
            _timer.Interval = 10_000;
            _timer.Elapsed += TimerElapsed;
            _timer.Start();

            TimerElapsed(_timer, null);

            StateHasChanged();
        }
    }

    private async void TimerElapsed(object? sender, ElapsedEventArgs? e)
    {
        try
        {
            using var response = await _http.GetAsync("http://127.0.0.1:41078/state");
            if (response.IsSuccessStatusCode)
            {
                var body = await response.Content.ReadAsStringAsync();
                _state = JsonSerializer.Deserialize<GameState>(body);

                switch (_state?.status ?? string.Empty)
                {
                    case "unknown": _status = EGameStatus.Unknown; break;
                    case "authorized": _status = EGameStatus.Authorized; break;
                    case "unauthorized": _status = EGameStatus.Unauthorized; break;
                    default: _status = EGameStatus.Failed; break;
                }
            }
            else
                _status = EGameStatus.Failed;
        }
        catch
        {
            _status = EGameStatus.NotConnected;
        }

        StateHasChanged();
    }

    public void Sync()
    {
        TimerElapsed(_timer, null);
    }

    private async Task LoginWithTwitch()
    {
        if (_isLogginigWithTwitch)
            return;
        _isLogginigWithTwitch = true;
        _isFailedToLoginWithTwitch = false;
        StateHasChanged();

        try
        {
            using var http = new HttpClient() { BaseAddress = new Uri(NavMgr.BaseUri) };
            using var response = await http.GetAsync("/Auth/OAuthEndpoint");
            response.EnsureSuccessStatusCode();
            var endpoint = await response.Content.ReadAsStringAsync();
            NavMgr.NavigateTo(endpoint, true);
        }
        catch (Exception e)
        {
            Console.Error.WriteLine(e.Message);
            _isFailedToLoginWithTwitch = true;
        }
        finally
        {
            _isLogginigWithTwitch = false;
            StateHasChanged();
        }
    }

    private record OAuthCodeResponse(string? token);

    private enum EGameStatus
    {
        Initial, NotConnected, Failed, Unknown, Unauthorized, Authorized
    }
}
