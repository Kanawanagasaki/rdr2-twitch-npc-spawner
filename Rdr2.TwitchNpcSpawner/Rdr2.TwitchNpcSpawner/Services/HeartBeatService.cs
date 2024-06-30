namespace Rdr2.TwitchNpcSpawner.Services;

using Rdr2.TwitchNpcSpawner.ApiClient;
using Rdr2.TwitchNpcSpawner.Entities;
using Microsoft.EntityFrameworkCore;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

public class HeartBeatService(IServiceProvider _serviceProvider, TwitchApiClient _twitchApi, ILogger<HeartBeatService> _logger, IConfiguration _conf) : BackgroundService
{
    private DateTimeOffset _lastBeat = DateTimeOffset.MinValue;

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            await Task.Delay(10000, stoppingToken);
            var now = DateTimeOffset.UtcNow;
            if (now - _lastBeat <= TimeSpan.FromMinutes(5) && !stoppingToken.IsCancellationRequested)
                continue;

            using var scope = _serviceProvider.CreateScope();
            using var db = scope.ServiceProvider.GetRequiredService<MySqlContext>();

            var cutoff = now - TimeSpan.FromMinutes(30);
            var cutoffSec = (long)(cutoff - DateTimeOffset.UnixEpoch).TotalSeconds;

            var beats = await db.HeartBeats.Where(x => x.LastBeatSec < cutoffSec).ToArrayAsync();

            foreach (var beat in beats)
            {
                var user = await db.Users.FirstOrDefaultAsync(x => x.Id == beat.UserId);
                if (user is null)
                    continue;

                var remoteRewardsRes = await _twitchApi.GetCustomRewardsList(user);
                if (remoteRewardsRes is not null && !remoteRewardsRes.IsSuccessStatusCode)
                {
                    if (await TryRefreshToken(db, user))
                        remoteRewardsRes = await _twitchApi.GetCustomRewardsList(user);
                    else
                        continue;
                }
                if (remoteRewardsRes is null || !remoteRewardsRes.IsSuccessStatusCode || remoteRewardsRes.data.Length == 0)
                    continue;

                var localRewards = await db.Rewards.Where(x => x.UserId == user.Id).ToArrayAsync();
                foreach (var localReward in localRewards)
                {
                    if (!localReward.IsCreated || localReward.TwitchId is null)
                        continue;

                    var remoteReward = remoteRewardsRes.data.FirstOrDefault(x => x.id == localReward.TwitchId);
                    if (remoteReward is null)
                        continue;

                    var remoteRewardReq = remoteReward.ToReq();
                    remoteRewardReq.is_enabled = false;

                    await _twitchApi.UpdateCustomReward(user, remoteReward.id, remoteRewardReq);
                }
            }

            db.HeartBeats.RemoveRange(beats);
            await db.SaveChangesAsync();

            _lastBeat = now;
        }

        _logger.LogInformation("HeartBeat service has stopped");
    }

    private record RefreshResult(string access_token, string refresh_token, string[] scope, string token_type);
    private async Task<bool> TryRefreshToken(MySqlContext db, UserEntity user)
    {
        using var http = new HttpClient();
        using var request = new HttpRequestMessage(HttpMethod.Get, "https://id.twitch.tv/oauth2/validate");
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        using var response = await http.SendAsync(request);

        if (response.StatusCode != System.Net.HttpStatusCode.OK)
        {
            var form = new FormUrlEncodedContent(new Dictionary<string, string?>
                {
                    { "client_id", _conf["Twitch:ClientId"] },
                    { "client_secret", _conf["Twitch:Secret"] },
                    { "grant_type", "refresh_token" },
                    { "refresh_token", user.RefreshToken }
                });
            using var request2 = new HttpRequestMessage(HttpMethod.Post, "https://id.twitch.tv/oauth2/token");
            request2.Content = form;
            using var response2 = await http.SendAsync(request2);
            if (response2.StatusCode != System.Net.HttpStatusCode.OK)
                return false;

            var content = await response2.Content.ReadAsStringAsync();
            var refreshRes = JsonSerializer.Deserialize<RefreshResult>(content);
            if (refreshRes is null)
                return false;

            user.AccessToken = refreshRes.access_token;
            user.RefreshToken = refreshRes.refresh_token;
            await db.SaveChangesAsync();
        }

        return true;
    }
}
