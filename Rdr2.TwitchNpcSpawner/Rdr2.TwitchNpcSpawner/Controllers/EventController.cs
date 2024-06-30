namespace Rdr2.TwitchNpcSpawner.Controllers;

using Microsoft.AspNetCore.Mvc;
using Microsoft.IdentityModel.Tokens;
using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text.Json;
using System.Text;
using Rdr2.TwitchNpcSpawner.Services;
using Microsoft.EntityFrameworkCore;
using Rdr2.TwitchNpcSpawner.ApiClient;

[ApiController, Route("[controller]")]
public class EventController(IConfiguration _conf, JwtService _jwtService) : Controller
{
    public record SubscribeRequest(string sessionId);
    [HttpPost("[action]")]
    public async Task<IActionResult> Subscribe([FromBody] SubscribeRequest req)
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        using var http = new HttpClient();
        using var request = new HttpRequestMessage(HttpMethod.Post, "https://api.twitch.tv/helix/eventsub/subscriptions");
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        request.Headers.Add("Client-Id", _conf["Twitch:ClientId"]);
        request.Content = JsonContent.Create(new
        {
            type = "channel.channel_points_custom_reward_redemption.add",
            version = "1",
            condition = new
            {
                broadcaster_user_id = user.TwitchId
            },
            transport = new
            {
                method = "websocket",
                session_id = req.sessionId
            }
        });
        using var response = await http.SendAsync(request);

        if (response.StatusCode != System.Net.HttpStatusCode.OK)
            return StatusCode((int)response.StatusCode);
        return NoContent();
    }
}
